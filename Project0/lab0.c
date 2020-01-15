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

void createSegfault()
{
    char *temp = NULL;
    *temp = 'x';
}

void handler()
{
    fprintf(stderr, "Segmentation fault caught.\n");
    exit(4);
}

int main(int argc, char **argv)
{
    char *input = NULL;
    char *output = NULL;
    int hasSegfault = 0;
    int hasCatch = 0;

    char param;

    struct option options[] =
        {
            {"input", 1, 0, 'i'},
            {"output", 1, 0, 'o'},
            {"segfault", 0, 0, 's'},
            {"catch", 0, 0, 'c'},
            {0, 0, 0, 0}};

    while (1)
    {
        param = getopt_long(argc, argv, "", options, NULL);
        if (param == -1)
        {
            break;
        }
        switch (param)
        {
        case 'i':
            input = optarg;
            break;
        case 'o':
            output = optarg;
            break;
        case 's':
            hasSegfault = 1;
            break;
        case 'c':
            hasCatch = 1;
            break;
        default:
            fprintf(stderr, "Incorrect usage! Correct usage is in the form: project0 --input=filepath --output=filepath --segfault --catch");
            exit(1);
            break;
        }
    }

    if (input)
    {
        int ifd = open(input, O_RDONLY);
        if (ifd >= 0)
        {
            close(0); // closes existing std in
            dup(ifd); // puts ifd into fd0
            close(ifd);
        }
        else
        {
            fprintf(stderr, "Error %s\n", strerror(errno));
            exit(2);
        }
    }

    if (output)
    {
        int ofd = creat(output, 0666); // creat(newfile, 0666) = open(newfile, O_CREAT, 0666)
        if (ofd >= 0)
        {
            close(1); // closes existing std out
            dup(ofd); // puts ofd in fd1
            close(ofd);
        }
        else
        {
            fprintf(stderr, "Error %s\n", strerror(errno));
            exit(3);
        }
    }

    if (hasCatch)
    {
        signal(SIGSEGV, handler);
    }

    if (hasSegfault)
    {
        createSegfault();
    }

    char buffer[1];
    int rd;
    while (1)
    {
        rd = read(0, buffer, 1);
        if (rd != 1)
        {
            break;
        }
        write(1, buffer, 1);
    }
    exit(0);
}
