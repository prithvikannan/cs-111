// NAME: Prithvi Kannan
// EMAIL: prithvi.kannan@gmail.com
// ID: 405110096

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <getopt.h>
#include <termios.h>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <poll.h>
#include <zlib.h>
#include <stdlib.h>

#define CR '\015'
#define LF '\012'
#define EOT '\004'
#define ESC '\003'

struct termios original_mode;
char crlf[2] = {CR, LF};

z_stream toServer;
z_stream toClient;

int sockfd, logfd;

void writeToLog(int compress, int sending, char* buf, int num) {
	char num_bytes[10];

	if (sending) {
		if (compress) {
			sprintf(num_bytes, "%d", 256 - toServer.avail_out);
		} else {
			sprintf(num_bytes, "%d", num);
		}
		write(logfd, "SENT ", 5);
		write(logfd, num_bytes, strlen(num_bytes));
		write(logfd, " bytes: ", 8);
		if (compress) {
			write(logfd, buf, 256 - toServer.avail_out);
		} else {
			write(logfd, buf, num);
		}
	} else {
		sprintf(num_bytes, "%d", num);
		write(logfd, "RECEIVED ", 9);
		write(logfd, num_bytes, strlen(num_bytes));
		write(logfd, " bytes: ", 8);
		write(logfd, buf, num);
	}
	write(logfd, "\n", 1);

}

void restore(void)
{
	tcsetattr(STDIN_FILENO, TCSANOW, &original_mode);
}

int main(int argc, char *argv[])
{

	// CHECK PARAMETERS

	struct option args[] = {
		{"port", 1, 0, 'p'},
		{"log", 1, 0, 'l'},
		{"compress", 0, 0, 'c'},
		{0, 0, 0, 0}};

	int portno = 0;
	int log = 0;
	int compress = 0;
	int param;

	while (1)
	{
		param = getopt_long(argc, argv, "", args, NULL);
		if (param == -1)
		{
			break;
		}
		switch (param)
		{
		case 'p':
			portno = atoi(optarg);
			break;

		case 'l':
			log = 1;
			logfd = creat(optarg, 0644);
			if (logfd == -1)
			{
				fprintf(stderr, "Error: unable to write to file\n\r");
			}
			break;

		case 'c':
			compress = 1;
			toClient.zalloc = Z_NULL;
			toClient.zfree = Z_NULL;
			toClient.opaque = Z_NULL;
			toServer.zalloc = Z_NULL;
			toServer.zfree = Z_NULL;
			toServer.opaque = Z_NULL;

			if (inflateInit(&toClient) != Z_OK)
			{
				fprintf(stderr, "Error: can't inflate on client side.\n\r");
				exit(1);
			}
			if (deflateInit(&toServer, Z_DEFAULT_COMPRESSION) != Z_OK)
			{
				fprintf(stderr, "Error: can't deflate on client side.\n\r");
				exit(1);
			}
			break;

		default:
			fprintf(stderr, "Error: expected usage is lab1b-client --port=port with optional --log=file and --compress.\n\r");
			exit(1);
			break;
		}
	}

	// CHANGE TERMINAL MODE

    struct termios modified_mode;
    if (!isatty(STDIN_FILENO))
    {
        fprintf(stderr, "Error: Standard input is not a terminal.\n");
        exit(1);
    }

    tcgetattr(STDIN_FILENO, &original_mode);
    tcgetattr(STDIN_FILENO, &modified_mode);
    modified_mode.c_iflag = ISTRIP;
    modified_mode.c_oflag = 0;
    modified_mode.c_lflag = 0;
    tcsetattr(STDIN_FILENO, TCSANOW, &modified_mode);
	atexit(restore);

	// MAKE A SOCKET

	struct sockaddr_in serv_addr;
	struct hostent *server;

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
		fprintf(stderr, "Error: can't open socket\n\r");
		exit(1);
	}
	server = gethostbyname("localhost");
	if (server == NULL)
	{
		fprintf(stderr, "Error: invalid host\n\r");
		exit(1);
	}
	bzero((char *)&serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	bcopy((char *)server->h_addr,
		  (char *)&serv_addr.sin_addr.s_addr,
		  server->h_length);
	if (portno == 0) {
		fprintf(stderr, "Error: invalid or no port\n\r");
	}
	serv_addr.sin_port = htons(portno);
	if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
	{
		fprintf(stderr, "Error: can't connect\n\r");
		exit(1);
	}

	// POLL FILE DESCRIPTORS

	struct pollfd file_descriptors[] = {
		{STDIN_FILENO, POLLIN, 0}, 
		{sockfd, POLLIN, 0}		   
	};

	int i;

	while (1)
	{
		if (poll(file_descriptors, 2, 0) > 0)
		{
			short revents_stdin = file_descriptors[0].revents;
			short revents_socket = file_descriptors[1].revents;

			if (revents_stdin == POLLIN)
			{
				char buf[256];
				int num = read(STDIN_FILENO, &buf, 256);
				i = 0;
				while (i < num)
				{
					switch(buf[i]) {
						case CR:
						case LF:
							write(STDOUT_FILENO, crlf, 2);
							break;
						case ESC:
							write(STDOUT_FILENO, "^C\r\n", 4);
							break;
						case EOT:
							write(STDOUT_FILENO, "^D\r\n", 4);
							break;
						default:
							write(STDOUT_FILENO, (buf + i), 1);
							break;
					}
					i++;
				}

				if (compress)
				{
					char bufCompress[256];
					toServer.avail_in = num;
					toServer.next_in = (unsigned char *)buf;
					toServer.avail_out = 256;
					toServer.next_out = (unsigned char *)bufCompress;

					do
					{
						deflate(&toServer, Z_SYNC_FLUSH);
					} while (toServer.avail_in > 0);

					write(sockfd, bufCompress, 256 - toServer.avail_out);

					if (log)
					{
						writeToLog(1, 1, bufCompress, 0);
					}
				}
				else
				{
					write(sockfd, buf, num);

					if (log)
					{
						writeToLog(0, 1, buf, num);
					}
				}
			}
			else if (revents_stdin == POLLERR)
			{
				fprintf(stderr, "Error: can't poll stdin.\n\r");
				exit(1);
			}

			if (revents_socket == POLLIN)
			{
				char buf[256];
				int num = read(sockfd, &buf, 256);
				if (num == 0)
				{
					break;
				}
				
				if (compress)
				{
					char bufCompress[1024];
					toClient.avail_in = num;
					toClient.next_in = (unsigned char *)buf;
					toClient.avail_out = 1024;
					toClient.next_out = (unsigned char *)bufCompress;

					do
					{
						inflate(&toClient, Z_SYNC_FLUSH);
					} while (toClient.avail_in > 0);

					write(STDOUT_FILENO, bufCompress, 1024 - toClient.avail_out);
				}
				else
				{
					write(STDOUT_FILENO, buf, num);
				}

				if (log)
				{
					writeToLog(0, 0, buf, num);
				}
			}
			else if (revents_socket & POLLERR || revents_socket & POLLHUP)
			{
				exit(0);
			}
		}
	}

	if (compress)
	{
		inflateEnd(&toClient);
		deflateEnd(&toServer);
	}

	close(sockfd);
	close(logfd);

	exit(0);
}