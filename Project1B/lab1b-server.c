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

#define CR '\015'  //carriage return
#define LF '\012'  //line feed
#define EOT '\004' //^D (End of transmission)
#define ETX '\003' //^C (End of text)

z_stream toServer;
z_stream toClient;

int pipe_parentToChild[2];
int pipe_childToParent[2];
int newPID;
const int READ_PORT = 0;
const int WRITE_PORT = 1;
int sockfd, newsockfd;
char crlf[2] = {CR, LF};

void handle_sigpipe()
{
    close(pipe_parentToChild[1]);
    close(pipe_childToParent[0]);
    kill(newPID, SIGKILL);
    int status;
    waitpid(newPID, &status, 0);
    fprintf(stderr, "SHELL EXIT SIGNAL=%d STATUS=%d\n", WTERMSIG(status), WEXITSTATUS(status));
    exit(0);
}

int main(int argc, char *argv[])
{
    /* supports --port and --compress args */

    struct option args[] = {

        {"port", 1, 0, 'p'},
        {"compress", 0, 0, 'c'},
        {0, 0, 0, 0}};

    int portno = 0;
    int compress = 0;
    int port_flag = 0;

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
            port_flag = 1;
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
                fprintf(stderr, "Failure to deflateInit on client side.\n");
                exit(1);
            }
            if (inflateInit(&toServer) != Z_OK)
            {
                fprintf(stderr, "Failure to inflateInit on client side.\n");
                exit(1);
            }

            break;

        default:
            fprintf(stderr, "Error in arguments.\n");
            exit(1);
            break;
        }
    }

    if (!port_flag)
    {
        fprintf(stderr, "--port= option is mandatory.\n");
        exit(1);
    }

    /* Create a socket */

    unsigned int clilen;
    struct sockaddr_in serv_addr, cli_addr;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        fprintf(stderr, "ERROR opening socket");
        exit(1);
    }
    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);
    if (bind(sockfd, (struct sockaddr *)&serv_addr,
             sizeof(serv_addr)) < 0)
    {
        fprintf(stderr, "ERROR on binding");
        exit(1);
    }
    listen(sockfd, 5);
    clilen = sizeof(cli_addr);
    newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
    if (newsockfd < 0)
    {
        fprintf(stderr, "ERROR on accept");
        exit(1);
    }

    //create pipes
    if (pipe(pipe_parentToChild) != 0 || pipe(pipe_childToParent) != 0)
    {
        fprintf(stderr, "Error: Failed to pipes between terminal and shell.\n");
        exit(1);
    }

    signal(SIGPIPE, handle_sigpipe);

    newPID = fork();

    switch (newPID)
    {
    case -1: // bad PID
        fprintf(stderr, "Error: Unable to fork process.\n");
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
            fprintf(stderr, "Error: Unable to execute shell.\n");
            exit(1);
        }
        break;

    default:
        close(pipe_parentToChild[0]); //read end from parent to child
        close(pipe_childToParent[1]); //write end from child to parent

        struct pollfd file_descriptors[] = {
            {newsockfd, POLLIN, 0},            //socket
            {pipe_childToParent[0], POLLIN, 0} //output from shell
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
                close(pipe_childToParent[0]);
                close(pipe_parentToChild[1]);
                fprintf(stderr, "SHELL EXIT SIGNAL=%d STATUS=%d\n", WTERMSIG(status), WEXITSTATUS(status));
                exit(0);
            }

            if (poll(file_descriptors, 2, 0) > 0)
            {

                short revents_socket = file_descriptors[0].revents;
                short revents_shell = file_descriptors[1].revents;

                /* check that socket has pending input */
                if (revents_socket == POLLIN)
                {
                    char input[256];
                    int num = read(newsockfd, &input, 256);

                    if (compress)
                    {
                        //decompress
                        //fprintf(stderr, "server decompressing data\n");
                        char compression_buf[1024];
                        toServer.avail_in = num;
                        toServer.next_in = (unsigned char *)input;
                        toServer.avail_out = 1024;
                        toServer.next_out = (unsigned char *)compression_buf;

                        do
                        {
                            inflate(&toServer, Z_SYNC_FLUSH);
                        } while (toServer.avail_in > 0);

                        for (i = 0; (unsigned int)i < 1024 - toServer.avail_out; i++)
                        {
                            if (compression_buf[i] == ETX)
                            {
                                kill(newPID, SIGINT);
                            }
                            else if (compression_buf[i] == EOT)
                            {
                                atEOT = 1;
                            }
                            else if (compression_buf[i] == CR || compression_buf[i] == LF)
                            {
                                char lf = LF;
                                write(pipe_parentToChild[1], &lf, 1);
                            }
                            else
                            {
                                write(pipe_parentToChild[1], (compression_buf + i), 1);
                            }
                        }
                    }
                    else
                    {
                        //no compress option
                        for (i = 0; i < num; i++)
                        {
                            if (input[i] == ETX)
                            {
                                kill(newPID, SIGINT);
                            }
                            else if (input[i] == EOT)
                            {
                                atEOT = 1;
                            }
                            else if (input[i] == CR || input[i] == LF)
                            {
                                char lf = LF;
                                write(pipe_parentToChild[1], &lf, 1);
                            }
                            else
                            {
                                write(pipe_parentToChild[1], (input + i), 1);
                            }
                        }
                    }
                }
                else if (revents_socket == POLLERR)
                {
                    fprintf(stderr, "Error with poll from socket.\n");
                    exit(1);
                }

                /* check that the shell has pending input */
                if (revents_shell == POLLIN)
                {
                    char input[256];
                    int num = read(pipe_childToParent[0], &input, 256);

                    int count = 0;
                    int j;
                    for (i = 0, j = 0; i < num; i++)
                    {
                        if (input[i] == EOT)
                        { //EOF from shell
                            atEOT = 1;
                        }
                        else if (input[i] == LF)
                        {

                            if (compress)
                            {
                                //fprintf(stderr, "server compressing data\n");
                                //compress
                                char compression_buf[256];
                                toClient.avail_in = count;
                                toClient.next_in = (unsigned char *)(input + j);
                                toClient.avail_out = 256;
                                toClient.next_out = (unsigned char *)compression_buf;

                                do
                                {
                                    deflate(&toClient, Z_SYNC_FLUSH);
                                } while (toClient.avail_in > 0);

                                write(newsockfd, compression_buf, 256 - toClient.avail_out);

                                //compress crlf
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
                                //no compress option
                                write(newsockfd, (input + j), count);
                                write(newsockfd, crlf, 2);
                            }

                            j += count + 1;
                            count = 0;
                            continue;
                        }
                        count++;
                    }

                    //compress
                    write(newsockfd, (input + j), count);
                }
                else if (revents_shell & POLLERR || revents_shell & POLLHUP)
                { //polling error
                    atEOT = 1;
                }
            }
        }

        close(sockfd);
        close(newsockfd);
        close(pipe_childToParent[0]);
        close(pipe_parentToChild[1]);
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