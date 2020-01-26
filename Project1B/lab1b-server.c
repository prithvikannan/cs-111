/* A simple server in the internet domain using TCP
   The port number is passed as an argument */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <getopt.h>
#include <sys/wait.h>
#include <poll.h>   

#define CR '\015'
#define LF '\012'
#define EOT '\004'
#define ESC '\003'

int pipe_parentToChild[2];
int pipe_childToParent[2];
int newPID;
const int READ_PORT = 0;
const int WRITE_PORT = 1;
int sockfd, newsockfd;
char crlf[2] = {CR, LF};

void dostuff(int); /* function prototype */

int main(int argc, char *argv[])
{

    int portno, clilen;
    char buffer[256];
    struct sockaddr_in serv_addr, cli_addr;
    int n;

    int param;
    struct option options[] =
        {
            {"port", 1, 0, 'p'},
            {0, 0, 0, 0}};

    if (argc != 2)
    {
        fprintf(stderr, "Error: Incorrect usage! Correct usage is in the form: lab1a OR lab1a --shell");
        exit(1);
    }

    param = getopt_long(argc, argv, "", options, NULL);
    switch (param)
    {
    case 'p':
        portno = atoi(optarg);
        break;
    default:
        fprintf(stderr, "Error: Incorrect usage! Correct usage is in the form: lab1a OR lab1a --shell");
        exit(1);
    }

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        fprintf(stderr, "ERROR opening socket");
    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);
    if (bind(sockfd, (struct sockaddr *)&serv_addr,
             sizeof(serv_addr)) < 0)
        fprintf(stderr, "ERROR on binding");
    listen(sockfd, 5);
    clilen = sizeof(cli_addr);
    newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
    if (newsockfd < 0)
        fprintf(stderr, "ERROR on accept");
    dostuff(newsockfd);
    return 0;
}

void dostuff(int sock)
{
    if (pipe(pipe_parentToChild) != 0 || pipe(pipe_childToParent) != 0)
    {
        fprintf(stderr, "Error: Failed to pipes between terminal and shell.\n");
        exit(1);
    }

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

    default:                                   // parent process
        close(pipe_parentToChild[READ_PORT]);  // close the read end of the parent pipe since we only want to write from the parent
        close(pipe_childToParent[WRITE_PORT]); // close the write end of the child pipe since we only want to read from the child

        // represents two ways of input: keyboard and output from shell
        struct pollfd file_descriptors[] = {
            {newsockfd, POLLIN, 0},
            {pipe_childToParent[READ_PORT], POLLIN, 0}};

        int atEOT = 0;
        int i;

        while (!atEOT)
        {
            if (poll(file_descriptors, 2, 0) > 0)
            {
                short socket_revents = file_descriptors[0].revents;
                short shell_revents = file_descriptors[1].revents;

                // if stdin has pending input
                if (socket_revents == POLLIN)
                {
                    char buf[256];
                    int num = read(newsockfd, &buf, 256);
                    i = 0;
                    while (i < num)
                    {
                        // ^C
                        if (buf[i] == ESC)
                        {
                            kill(newPID, SIGINT);
                        }
                        // ^D
                        else if (buf[i] == EOT)
                        {
                            atEOT = 1;
                        }
                        // carriage return or line feed from stdin
                        else if (buf[i] == CR || buf[i] == LF)
                        {
                            char lf = LF;
                            write(pipe_parentToChild[WRITE_PORT], &lf, 1);
                        }
                        // regular char from stdn
                        else
                        {
                            write(pipe_parentToChild[WRITE_PORT], (buf + i), 1);
                        }

                        i++;
                    }
                }
                if (socket_revents == POLLERR || socket_revents == POLLHUP)
                {
                    fprintf(stderr, "Error: Unable to poll stdin.\n");
                    exit(1);
                }

                // if shell has pending input
                if (shell_revents == POLLIN)
                {
                    char buf[256];
                    int num = read(pipe_childToParent[READ_PORT], &buf, 256);
                    int count = 0;
                    int j;
                    i = 0;
                    j = 0;
                    while (i < num)
                    {
                        // end of file from shell
                        if (buf[i] == EOT)
                        {
                            atEOT = 1;
                        }
                        // line feed from shell
                        else if (buf[i] == LF)
                        {
                            write(newsockfd, (buf + j), count);
                            write(newsockfd, crlf, 2);
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

                // end of file from shell
                else if (shell_revents & POLLERR || shell_revents & POLLHUP)
                {
                    atEOT = 1;
                }
            }
        }

        close(sockfd);
        close(newsockfd);

        close(pipe_childToParent[READ_PORT]);  // close the read end of the child pipe once we are done
        close(pipe_parentToChild[WRITE_PORT]); // close the write end of the child pipe once we are done

        // wait for child thread to terminate
        int wstatus;
        waitpid(newPID, &wstatus, 0);
        fprintf(stderr, "SHELL EXIT SIGNAL=%d STATUS=%d\r\n", WTERMSIG(wstatus), WEXITSTATUS(wstatus));

        exit(0);
    }
}
