// NAME: Prithvi Kannan
// EMAIL: prithvi.kannan@gmail.com
// ID: 405110096

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <fcntl.h>
#include <getopt.h>
#include <unistd.h>
#include <string.h>
#include <termios.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <poll.h>   

#define CR '\015'
#define LF '\012'
#define EOT '\004'
#define ESC '\003'

struct termios original_mode;
int pipe_parentToChild[2];
int pipe_childToParent[2];
int newPID;
const int READ_PORT = 0;
const int WRITE_PORT = 1;

char crlf[2] = {CR, LF};
char ctrlD[2] = {'^', 'D'};
char ctrlC[2] = {'^', 'C'};

void restore(void)
{
    tcsetattr(STDIN_FILENO, TCSANOW, &original_mode);
}

int main(int argc, char **argv)
{
    int shell;
    shell = 0;
    int param;
    struct option options[] =
        {
            {"shell", 0, 0, 's'},
            {0, 0, 0, 0}};

    if (argc != 1 && argc != 2)
    {
        fprintf(stderr, "Error: Incorrect usage! Correct usage is in the form: lab1a OR lab1a --shell");
        exit(1);
    }
    while (1)
    {
        param = getopt_long(argc, argv, "", options, NULL);
        if (param == -1)
        {
            break;
        }
        switch (param)
        {
        case 's':
            shell = 1;
            break;
        default:
            fprintf(stderr, "Error: Incorrect usage! Correct usage is in the form: lab1a OR lab1a --shell");
            exit(1);
            break;
        }
    }

    // change modes
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

    // if shell flag is given
    if (shell)
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
                {STDIN_FILENO, POLLIN, 0},
                {pipe_childToParent[READ_PORT], POLLIN, 0}};

            int atEOT = 0;
            int i;

            while (!atEOT)
            {
                if (poll(file_descriptors, 2, 0) > 0)
                {
                    short stdin_revents = file_descriptors[0].revents;
                    short shell_revents = file_descriptors[1].revents;

                    // if stdin has pending input
                    if (stdin_revents == POLLIN)
                    {
                        char buf[256];
                        int num = read(STDIN_FILENO, &buf, 256);
                        i = 0;
                        while (i < num)
                        {
                            // ^C from stdin
                            if (buf[i] == ESC)
                            {
                                write(STDOUT_FILENO, &ctrlC, 2);
                                kill(newPID, SIGINT);
                            }
                            // ^D from stdin
                            else if (buf[i] == EOT)
                            {
                                write(STDOUT_FILENO, &crlf, 2);
                                write(STDOUT_FILENO, &ctrlD, 2);

                                atEOT = 1;
                            }
                            // carriage return or line feed from stdin
                            else if (buf[i] == CR || buf[i] == LF)
                            {
                                char lf = LF;
                                write(STDOUT_FILENO, crlf, 2);
                                write(pipe_parentToChild[WRITE_PORT], &lf, 1);
                            }
                            // regular char from stdn
                            else
                            {
                                write(STDOUT_FILENO, (buf + i), 1);
                                write(pipe_parentToChild[WRITE_PORT], (buf + i), 1);
                            }

                            i++;
                        }
                    }
                    if (stdin_revents == POLLERR || stdin_revents == POLLHUP)
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
                                write(STDOUT_FILENO, (buf + j), count);
                                write(STDOUT_FILENO, crlf, 2);
                                j += count + 1;
                                count = 0;
                                i++;
                                continue;
                            }
                            count++;
                            i++;
                        }
                        write(STDOUT_FILENO, (buf + j), count);
                    }

                    // end of file from shell
                    else if (shell_revents & POLLERR || shell_revents & POLLHUP)
                    {
                        atEOT = 1;
                    }
                }
            }

            close(pipe_childToParent[READ_PORT]);  // close the read end of the child pipe once we are done
            close(pipe_parentToChild[WRITE_PORT]); // close the write end of the child pipe once we are done

            // wait for child thread to terminate
            int wstatus;
            waitpid(newPID, &wstatus, 0);
            fprintf(stderr, "SHELL EXIT SIGNAL=%d STATUS=%d\r\n", WTERMSIG(wstatus), WEXITSTATUS(wstatus));

            atexit(restore);
            exit(0);
        }
    }

    // if no shell flag
    else
    {
        char buf[256];
        int amountRead;
        int exitLoop;
        exitLoop = 0;
        while ((amountRead = read(STDIN_FILENO, buf, 256)) > 0)
        {
            int a = 0;
            while (a < amountRead)
            {
                switch (buf[a])
                {
                case ESC:
                    write(STDOUT_FILENO, &ctrlC, 2);
                    break;
                case EOT:
                    exitLoop = 1;
                    write(STDOUT_FILENO, &crlf, 2);
                    write(STDOUT_FILENO, &ctrlD, 2);
                    break;
                case CR:
                case LF:
                    write(STDOUT_FILENO, crlf, 2);
                    break;
                default:
                    write(STDOUT_FILENO, &buf[a], 1);
                    break;
                }
                a++;
                if (exitLoop)
                {
                    break;
                }
            }
            if (exitLoop)
            {
                break;
            }
        }

        atexit(restore);
    }
    exit(0);
}
