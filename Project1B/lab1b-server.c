// NAME: Prithvi Kannan
// EMAIL: prithvi.kannan@gmail.com
// ID: 405110096

#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <getopt.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <assert.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <poll.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <netdb.h>
#include <netinet/in.h>
#include <string.h>
#include <zlib.h>

#define CR '\015'
#define LF '\012'
#define EOT '\004'
#define ESC '\003'

z_stream toServer;
z_stream toClient;

int pipe_parentToChild[2];
int pipe_childToParent[2];
int newPID;
const int READ_PORT = 0;
const int WRITE_PORT = 1;
int sockfd, newsockfd;
char crlf[2] = {CR, LF};
char lf = LF;

void handle_character(char *buf, int i, int* atEOT) {
    switch(buf[i]) 
    {
        case ESC:
            kill(newPID, SIGINT);
            break;
        case EOT:
            *(atEOT) = 1;
            break;
        case CR:
        case LF:
            write(pipe_parentToChild[WRITE_PORT], &lf, 1);
            break;
        default:
            write(pipe_parentToChild[WRITE_PORT], (buf + i), 1);
            break;
    }
}
void handleSignal()
{
    kill(newPID, SIGKILL);
    close(pipe_parentToChild[WRITE_PORT]);
    close(pipe_childToParent[READ_PORT]);
    int status;
    waitpid(newPID, &status, 0);
    fprintf(stderr, "SHELL EXIT SIGNAL=%d STATUS=%d\n", WTERMSIG(status), WEXITSTATUS(status));
    exit(0);
}

int main(int argc, char *argv[])
{

    // CHECK PARAMETERS

    struct option args[] = {

        {"port", 1, 0, 'p'},
        {"compress", 0, 0, 'c'},
        {0, 0, 0, 0}};

    int portno = 0;
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

        case 'c':
            compress = 1;

            toClient.zalloc = Z_NULL;
            toClient.zfree = Z_NULL;
            toClient.opaque = Z_NULL;
            toServer.zalloc = Z_NULL;
            toServer.zfree = Z_NULL;
            toServer.opaque = Z_NULL;

            if (deflateInit(&toClient, Z_DEFAULT_COMPRESSION) != Z_OK)
            {
				fprintf(stderr, "Error: can't deflate on client side.\n\r");
                exit(1);
            }
            if (inflateInit(&toServer) != Z_OK)
            {
				fprintf(stderr, "Error: can't inflate on client side.\n\r");
                exit(1);
            }
            break;

        default:
			fprintf(stderr, "Error: expected usage is lab1b-server --port=port with optional --compress.\n\r");
            exit(1);
            break;
        }
    }

	// MAKE A SOCKET

    unsigned int clilen;
    struct sockaddr_in serv_addr, cli_addr;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
		fprintf(stderr, "Error: can't open socket\n\r");
        exit(1);
    }
    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    if (portno == 0) {
		fprintf(stderr, "Error: invalid or no port\n\r");
	}
    serv_addr.sin_port = htons(portno);
    if (bind(sockfd, (struct sockaddr *)&serv_addr,
             sizeof(serv_addr)) < 0)
    {
		fprintf(stderr, "Error: can't bind socket\n\r");
        exit(1);
    }
    listen(sockfd, 5);
    clilen = sizeof(cli_addr);
    newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
    if (newsockfd < 0)
    {
		fprintf(stderr, "Error: unable to accept\n\r");
        exit(1);
    }

    // MAKE PIPES

    if (pipe(pipe_parentToChild) != 0 || pipe(pipe_childToParent) != 0)
    {
		fprintf(stderr, "Error: failed to make pipes\n\r");
        exit(1);
    }

    signal(SIGPIPE, handleSignal);

    newPID = fork();

    switch (newPID)
    {
    case -1: // bad pid
		fprintf(stderr, "Error: unable to fork\n\r");
        exit(1);

    case 0:                                    // child process
        close(pipe_parentToChild[WRITE_PORT]); // close the write end of the parent pipe since we only want to read from the parent
        close(pipe_childToParent[READ_PORT]);  // close the read end of the child pipe since we only want to write from the child

        dup2(pipe_parentToChild[READ_PORT], STDIN_FILENO);   // copies the read end of the parent pipe to standard in
        dup2(pipe_childToParent[WRITE_PORT], STDOUT_FILENO); // copies the write end of the child pipe to standard out
        dup2(pipe_childToParent[WRITE_PORT], STDERR_FILENO); // copies the write end of the child pipe to standard err

        close(pipe_parentToChild[READ_PORT]);  // closes extra copy of the the read end of parent pipe
        close(pipe_childToParent[WRITE_PORT]); // closes extra copy of the the write end of child pipe

        char *args[2];
        char name[] = "/bin/bash";
        args[0] = name;
        args[1] = NULL;
        if (execvp(name, args) == -1)
        {
		fprintf(stderr, "Error: can't execute shell\n\r");
            exit(1);
        }
        break;

    default: // parent process
        close(pipe_parentToChild[READ_PORT]); //read end from parent to child
        close(pipe_childToParent[WRITE_PORT]); //write end from child to parent

        struct pollfd file_descriptors[] = {
            {newsockfd, POLLIN, 0},            //socket
            {pipe_childToParent[READ_PORT], POLLIN, 0} //output from shell
        };

        int atEOT = 0;
        int i;

        int status;
        while (!atEOT)
        {

            if (waitpid(newPID, &status, WNOHANG) != 0)
            {
                close(sockfd);
                close(newsockfd);
                close(pipe_childToParent[READ_PORT]);
                close(pipe_parentToChild[WRITE_PORT]);
                fprintf(stderr, "SHELL EXIT SIGNAL=%d STATUS=%d\n", WTERMSIG(status), WEXITSTATUS(status));
                exit(0);
            }

            if (poll(file_descriptors, 2, 0) > 0)
            {

                short socket_revents = file_descriptors[0].revents;
                short shell_revents = file_descriptors[1].revents;

                if (socket_revents == POLLIN)
                {
                    char buf[256];
                    int num = read(newsockfd, &buf, 256);

                    if (compress)
                    {
                        char compression_buf[1024];
                        toServer.avail_in = num;
                        toServer.next_in = (unsigned char *)buf;
                        toServer.avail_out = 1024;
                        toServer.next_out = (unsigned char *)compression_buf;

                        do
                        {
                            inflate(&toServer, Z_SYNC_FLUSH);
                        } while (toServer.avail_in > 0);

                        i = 0;
                        while ((unsigned int)i < 1024 - toServer.avail_out)
                        {
                            handle_character(compression_buf+i, i, &atEOT);
                            i++;
                        }
                    }
                    else
                    {
                        i = 0;
                        while (i < num)
                        {
                            handle_character(buf+i, i, &atEOT);
                            i++;
                        }
                    }
                }
                else if (socket_revents == POLLERR)
                {
                    fprintf(stderr, "Error: can't poll socket.\n\r");
                    exit(1);
                }

                if (shell_revents == POLLIN)
                {
                    char buf[256];
                    int num = read(pipe_childToParent[READ_PORT], &buf, 256);
                    int count = 0;
                    int j;
                    i = 0;
                    j = 0;
                    while ( i < num)
                    {
                        if (buf[i] == EOT)
                        { 
                            atEOT = 1;
                        }
                        else if (buf[i] == LF)
                        {

                            if (compress)
                            {
                                char compression_buf[256];
                                toClient.avail_in = count;
                                toClient.next_in = (unsigned char *)(buf + j);
                                toClient.avail_out = 256;
                                toClient.next_out = (unsigned char *)compression_buf;

                                do
                                {
                                    deflate(&toClient, Z_SYNC_FLUSH);
                                } while (toClient.avail_in > 0);

                                write(newsockfd, compression_buf, 256 - toClient.avail_out);

                                char compression_buf2[256];
                                char crlf_copy[2] = {CR, LF};
                                toClient.avail_in = 2;
                                toClient.next_in = (unsigned char *)(crlf_copy);
                                toClient.avail_out = 256;
                                toClient.next_out = (unsigned char *)compression_buf2;

                                do
                                {
                                    deflate(&toClient, Z_SYNC_FLUSH);
                                } while (toClient.avail_in > 0);

                                write(newsockfd, compression_buf2, 256 - toClient.avail_out);
                            }
                            else
                            {
                                write(newsockfd, (buf + j), count);
                                write(newsockfd, crlf, 2);
                            }

                            j += count + 1;
                            count = 0;
                            i++;
                            continue;
                        }
                        count++;
                        i++;
                    }

                    write(newsockfd, (buf + j), count);
                }
                else if (shell_revents & POLLERR || shell_revents & POLLHUP)
                { 
                    atEOT = 1;
                }
            }
        }

        close(sockfd);
        close(newsockfd);
        close(pipe_childToParent[READ_PORT]);
        close(pipe_parentToChild[WRITE_PORT]);
        int tempStatus;
        waitpid(newPID, &tempStatus, 0);
        fprintf(stderr, "SHELL EXIT SIGNAL=%d STATUS=%d\n", WTERMSIG(tempStatus), WEXITSTATUS(tempStatus));
    }

    if (compress)
    {
        inflateEnd(&toServer);
        deflateEnd(&toClient);
    }
    exit(0);
}