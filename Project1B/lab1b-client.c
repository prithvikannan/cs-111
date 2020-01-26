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

#define CR '\015'
#define LF '\012'
#define EOT '\004'
#define ESC '\003'

struct termios original_mode;
char crlf[2] = {CR, LF};

void restore(void)
{
    tcsetattr(STDIN_FILENO, TCSANOW, &original_mode);
}

int main(int argc, char *argv[])
{
    int sockfd, portno, n;

    struct sockaddr_in serv_addr;
    struct hostent *server;

    char buffer[256];

    int param;
    char *filename = NULL;
    int compress = 0;
    struct option options[] =
        {
            {"port", 1, 0, 'p'},
            {"log", 1, 0, 'l'},
            {"compress", 0, 0, 'c'},
            {0, 0, 0, 0}};

    if (argc != 2 && argc != 3 && argc != 4)
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
        case 'p':
            portno = atoi(optarg);
            break;
        case 'l':
            filename = optarg;
            break;
        case 'c':
            compress = 1;
            break;
        default:
            fprintf(stderr, "Error: Incorrect usage! Correct usage is in the form: lab1a OR lab1a --shell");
            exit(1);
            break;
        }
    }

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

    struct pollfd file_descriptors[] = {
        {STDIN_FILENO, POLLIN, 0}, //keyboard (stdin)
        {sockfd, POLLIN, 0}        //socket
    };

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
                int i;
                for (i = 0; i < num; i++)
                {
                    if (input[i] == CR || input[i] == LF)
                    {
                        write(STDOUT_FILENO, crlf, 2);
                    }
                    else
                    {
                        write(STDOUT_FILENO, (input + i), 1);
                    }
                }
                write(sockfd, input, num);
            }
            else if (revents_stdin == POLLERR)
            {
                fprintf(stderr, "Error with poll with STDIN.\n");
                exit(1);
            }

            if (revents_socket == POLLIN)
            {
                char input[256];
                int num = read(sockfd, &input, 256);
                if (num == 0)
                {
                    break;
                }
                write(STDOUT_FILENO, input, num);
            }
            else if (revents_socket & POLLERR || revents_socket & POLLHUP)
            { //polling error
                exit(0);
            }
        }
    }

    atexit(restore);

    return 0;
}