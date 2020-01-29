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
char newline = '\n';

char sending_prefix[20] = "SENT ";
char sending_end[20] = " bytes: ";
char receiving_prefix[20] = "RECEIVED ";
char receiving_end[20] = " bytes: ";

z_stream toServer;
z_stream toClient;

int sockfd, logfd;

//the function that will be called upon normal process termination
void restore(void)
{
	tcsetattr(STDIN_FILENO, TCSANOW, &original_mode);
}

/* Puts the standard input into non-canonical input mode with no echo */
void set_mode(void)
{
	struct termios mode;

	if (!isatty(STDIN_FILENO))
	{
		fprintf(stderr, "Standard input does not refer to a terminal.\n\r");
		exit(1);
	}

	tcgetattr(STDIN_FILENO, &original_mode);
	atexit(restore);

	tcgetattr(STDIN_FILENO, &mode);

	mode.c_iflag = ISTRIP;
	mode.c_oflag = 0;
	mode.c_lflag = 0;
	mode.c_cc[VMIN] = 1;
	mode.c_cc[VTIME] = 0;

	tcsetattr(STDIN_FILENO, TCSANOW, &mode);
}

int main(int argc, char *argv[])
{
	/* supports --log, --port, and --compress arguments */
	struct option args[] = {
		{"port", 1, 0, 'p'},
		{"log", 1, 0, 'l'},
		{"compress", 0, 0, 'c'},
		{0, 0, 0, 0}};

	int portno = 0;
	int log = 0;
	logfd = -1;
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
			if ((logfd = creat(optarg, 0666)) == -1)
			{
				fprintf(stderr, "Failure to create/write to file.\n\r");
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
				fprintf(stderr, "Failure to inflateInit on client side.\n\r");
				exit(1);
			}

			if (deflateInit(&toServer, Z_DEFAULT_COMPRESSION) != Z_OK)
			{
				fprintf(stderr, "Failure to deflateInit on client side.\n\r");
				exit(1);
			}

			break;

		default:
			fprintf(stderr, "Error in arguments.\n\r");
			exit(1);
			break;
		}
	}

	set_mode();

	/* Create a socket */

	struct sockaddr_in serv_addr;
	struct hostent *server;

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0)
		fprintf(stderr, "ERROR opening socket");
	server = gethostbyname("localhost");
	if (server == NULL)
	{
		fprintf(stderr, "ERROR, no such host\n");
		exit(0);
	}
	bzero((char *)&serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	bcopy((char *)server->h_addr,
		  (char *)&serv_addr.sin_addr.s_addr,
		  server->h_length);
	serv_addr.sin_port = htons(portno);
	if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
	{
		fprintf(stderr, "ERROR connecting");
		exit(1);
	}

	struct pollfd file_descriptors[] = {
		{STDIN_FILENO, POLLIN, 0}, //keyboard (stdin)
		{sockfd, POLLIN, 0}		   //socket
	};

	int i;
	char sending_prefix[20] = "SENT ";
	char sending_end[20] = " bytes: ";
	char receiving_prefix[20] = "RECEIVED ";
	char receiving_end[20] = " bytes: ";

	while (1)
	{
		if (poll(file_descriptors, 2, 0) > 0)
		{
			short revents_stdin = file_descriptors[0].revents;
			short revents_socket = file_descriptors[1].revents;

			/* check that stdin has pending input */
			if (revents_stdin == POLLIN)
			{
				char input[256];
				int num = read(STDIN_FILENO, &input, 256);
				i = 0;
				while (i < num)
				{
					if (input[i] == CR || input[i] == LF)
					{
						write(STDOUT_FILENO, crlf, 2);
					}
					else
					{
						write(STDOUT_FILENO, (input + i), 1);
					}
					i++;
				}

				if (compress)
				{
					//compress data
					//fprintf(stderr, "client compressing data\n");
					char compression_buf[256];
					toServer.avail_in = num;
					toServer.next_in = (unsigned char *)input;
					toServer.avail_out = 256;
					toServer.next_out = (unsigned char *)compression_buf;

					do
					{
						deflate(&toServer, Z_SYNC_FLUSH);
					} while (toServer.avail_in > 0);

					write(sockfd, compression_buf, 256 - toServer.avail_out);

					if (log)
					{
						char num_bytes[20];
						sprintf(num_bytes, "%d", 256 - toServer.avail_out);
						write(logfd, sending_prefix, strlen(sending_prefix));
						write(logfd, num_bytes, strlen(num_bytes));
						write(logfd, sending_end, strlen(sending_end));
						write(logfd, compression_buf, 256 - toServer.avail_out);
						write(logfd, &newline, 1);
					}
				}
				else
				{
					//no compress option
					write(sockfd, input, num);

					if (log)
					{
						char num_bytes[20];
						sprintf(num_bytes, "%d", num);
						write(logfd, sending_prefix, strlen(sending_prefix));
						write(logfd, num_bytes, strlen(num_bytes));
						write(logfd, sending_end, strlen(sending_end));
						write(logfd, input, num);
						write(logfd, &newline, 1);
					}
				}
			}
			else if (revents_stdin == POLLERR)
			{
				fprintf(stderr, "Error with poll with STDIN.\n\r");
				exit(1);
			}

			/* check that the socket has pending input */
			if (revents_socket == POLLIN)
			{
				char input[256];
				int num = read(sockfd, &input, 256);
				if (num == 0)
				{
					break;
				}

				if (compress)
				{
					//decompress data
					//fprintf(stderr, "client decompressing data\n");
					char compression_buf[1024];
					toClient.avail_in = num;
					toClient.next_in = (unsigned char *)input;
					toClient.avail_out = 1024;
					toClient.next_out = (unsigned char *)compression_buf;

					do
					{
						inflate(&toClient, Z_SYNC_FLUSH);
					} while (toClient.avail_in > 0);

					write(STDOUT_FILENO, compression_buf, 1024 - toClient.avail_out);
				}
				else
				{
					// no compress option
					write(STDOUT_FILENO, input, num);
				}

				if (log)
				{
					char num_bytes[20];
					sprintf(num_bytes, "%d", num);
					write(logfd, receiving_prefix, strlen(receiving_prefix));
					write(logfd, num_bytes, strlen(num_bytes));
					write(logfd, receiving_end, strlen(receiving_end));
					write(logfd, input, num);
					write(logfd, &newline, 1);
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