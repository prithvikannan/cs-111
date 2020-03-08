// NAME: Prithvi Kannan
// EMAIL: prithvi.kannan@gmail.com
// ID: 405110096

#define _POSIX_C_SOURCE 200809 // to get rid of dprintf warnings
#define h_addr h_addr_list[0]  /* for backward compatibility */

#include <unistd.h>
#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <poll.h>
#include <time.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <ctype.h>
#include <mraa.h>
#include <mraa/aio.h>
#include "fcntl.h"
#include <netinet/in.h>
#include <netdb.h>

#if DEV
const int mraaFlag = 0;
#else
const int mraaFlag = 1;
#endif

const int BETA = 4275;
const int R0 = 100000;

int period = 1;
char unit;

struct pollfd myPoll[1];
int fd;
int shouldLog = 0;

mraa_aio_context temperatureSensor;

int noReports = 0;

int portNum;
int socketFd = 0;
struct sockaddr_in address;
struct hostent *server;
char *hostname = NULL;
char *idNum;

void print_errors(char *error)
{
    if (strcmp(error, "temp") == 0)
    {
        fprintf(stderr, "Failed to initialize temperature sensor\n");
        mraa_deinit();
        exit(1);
    }
    if (strcmp(error, "usage") == 0)
    {
        fprintf(stderr, "Incorrect argument: correct usage is ./lab4a --period=# [--scale=tempOpt] [--log=filename]\n");
        exit(1);
    }
    if (strcmp(error, "file") == 0)
    {
        fprintf(stderr, "Failed to create file\n");
        exit(2);
    }
    if (strcmp(error, "poll") == 0)
    {
        fprintf(stderr, "Failed to poll\n");
        exit(2);
    }
    if (strcmp(error, "read") == 0)
    {
        fprintf(stderr, "Failed to read\n");
        exit(2);
    }
    if (strcmp(error, "period") == 0)
    {
        fprintf(stderr, "Period of 0 is not legal\n");
        exit(1);
    }
    if (strcmp(error, "socket") == 0)
    {
        fprintf(stderr, "Error creating socket\n");
        exit(2);
    }
    if (strcmp(error, "connection") == 0)
    {
        fprintf(stderr, "Error establishing connection to server\n");
        exit(2);
    }
    if (strcmp(error, "id_length") == 0)
    {
        fprintf(stderr, "ID length is not 9. Inavlid ID\n");
        exit(1);
    }
    if (strcmp(error, "host") == 0)
    {
        fprintf(stderr, "failed to get host name\n");
        exit(1);
    }
}

void create_report(double temp)
{
    time_t raw;
    struct tm *currTime;
    time(&raw);
    currTime = localtime(&raw);
    dprintf(socketFd, "%.2d:%.2d:%.2d %.1f\n", currTime->tm_hour, currTime->tm_min, currTime->tm_sec, temp);
    if (shouldLog && noReports == 0)
    {
        dprintf(fd, "%.2d:%.2d:%.2d %.1f\n", currTime->tm_hour, currTime->tm_min, currTime->tm_sec, temp);
    }
} //help with time from https://www.tutorialspoint.com/c_standard_library/c_function_localtime.htm

double convertTemp(int temperatureSensorInput)
{
    double temp = 1023.0 / (double)temperatureSensorInput - 1.0;
    temp *= R0;
    float temperature = 1.0 / (log(temp / R0) / BETA + 1 / 298.15) - 273.15;
    if (unit == 'C')
    {
        return temperature;
    }
    else
    {
        return temperature * 9 / 5 + 32;
    }
}

void handle_scale(char scale)
{
    switch (scale)
    {
    case 'C':
    case 'c':
        unit = 'C';
        if (shouldLog && noReports == 0)
        {
            dprintf(fd, "SCALE=C\n");
        }
        break;
    case 'F':
    case 'f':
        unit = 'F';
        if (shouldLog && noReports == 0)
        {
            dprintf(fd, "SCALE=F\n");
        }
        break;
    default:
        fprintf(stderr, "Incorrect scale option");
        break;
    }
}

void readInput(const char *input)
{
    if (!strcmp(input, "OFF"))
    {
        if (shouldLog)
        {
            dprintf(fd, "OFF\n");
        }
        time_t raw;
        struct tm *currTime;
        time(&raw);
        currTime = localtime(&raw);
        dprintf(socketFd, "%.2d:%.2d:%.2d SHUTDOWN\n", currTime->tm_hour, currTime->tm_min, currTime->tm_sec);
        if (shouldLog)
        {
            dprintf(fd, "%.2d:%.2d:%.2d SHUTDOWN\n", currTime->tm_hour, currTime->tm_min, currTime->tm_sec);
        }
        exit(0);
    }
    else if (!strcmp(input, "START"))
    {
        noReports = 0;
        if (shouldLog)
        {
            dprintf(fd, "START\n");
        }
    }
    else if (!strcmp(input, "STOP"))
    {
        noReports = 1;
        if (shouldLog)
        {
            dprintf(fd, "STOP\n");
        }
    }
    else if (!strcmp(input, "SCALE=F"))
    {
        unit = 'F';
        if (shouldLog && noReports == 0)
        {
            dprintf(fd, "SCALE=F\n");
        }
    }
    else if (!strcmp(input, "SCALE=C"))
    {
        unit = 'C';
        if (shouldLog && noReports == 0)
        {
            dprintf(fd, "SCALE=C\n");
        }
    }
    else if (!strncmp(input, "PERIOD=", sizeof(char) * 7))
    {
        period = (int)atoi(input + 7);
        if (shouldLog && noReports == 0)
        {
            dprintf(fd, "PERIOD=%d\n", period);
        }
    }
    else if (!strncmp(input, "LOG", sizeof(char) * 3))
    {
        if (shouldLog)
        {
            dprintf(fd, "%s\n", input);
        }
    }
    else
    {
        fprintf(stderr, "Command not recognized\n");
        exit(1);
    }
}

void setupPollandTime()
{
    char commandBuff[128];
    char copyBuff[128];
    memset(commandBuff, 0, 128);
    memset(copyBuff, 0, 128);
    int copyIndex = 0;
    myPoll[0].fd = socketFd;
    myPoll[0].events = POLLIN | POLLERR | POLLHUP;
    for (;;)
    {
        int value = mraa_aio_read(temperatureSensor);
        double tempValue = convertTemp(value);
        if (!noReports)
        {
            create_report(tempValue);
        }
        time_t begin, end;
        time(&begin);
        time(&end); //start begin and end at the same time and keep running loop until period is reached
        while (difftime(end, begin) < period)
        {
            int ret = poll(myPoll, 1, 0);
            if (ret < 0)
            {
                print_errors("poll");
            }
            if (myPoll[0].revents && POLLIN)
            {
                int num = read(socketFd, commandBuff, 128);
                if (num < 0)
                {
                    print_errors("read");
                }
                int i;
                for (i = 0; i < num && copyIndex < 128; i++)
                {
                    if (commandBuff[i] == '\n')
                    {
                        readInput((char *)&copyBuff);
                        copyIndex = 0;
                        memset(copyBuff, 0, 128); //clear
                    }
                    else
                    {
                        copyBuff[copyIndex] = commandBuff[i];
                        copyIndex++;
                    }
                }
            }
            time(&end);
        }
    }
} //help with time https://www.tutorialspoint.com/c_standard_library/c_function_time.htm

int main(int argc, char **argv)
{
    int opt = 0;
    static struct option options[] = {
        {"period", 1, 0, 'p'},
        {"scale", 1, 0, 's'},
        {"log", 1, 0, 'l'},
        {"id", 1, 0, 'i'},
        {"host", 1, 0, 'h'},
        {0, 0, 0, 0}};

    while ((opt = getopt_long(argc, argv, "p:sl", options, NULL)) != -1)
    {
        switch (opt)
        {
        case 'p':
            period = (int)atoi(optarg);
            if (period == 0)
            {
                print_errors("period");
            }
            break;
        case 's':
            switch (*optarg)
            {
            case 'C':
            case 'c':
                unit = 'C';
                break;
            case 'F':
            case 'f':
                unit = 'F';
                break;
            default:
                print_errors("usage");
                break;
            }
            break;
        case 'l':
            shouldLog = 1;
            char *logFile = optarg;
            fd = creat(logFile, 0666);
            if (fd < 0)
            {
                print_errors("file");
            }
            break;
        case 'i':
            if (strlen(optarg) != 9)
            {
                print_errors("id_length");
            }
            idNum = optarg;
            break;
        case 'h':
            hostname = optarg;
            break;
        default:
            print_errors("usage");
            break;
        }
    }
    portNum = atoi(argv[optind]);
    if (portNum <= 0)
    {
        fprintf(stderr, "Invalid port number\n");
        exit(1);
    }
    close(STDIN_FILENO); //close input

    socketFd = socket(AF_INET, SOCK_STREAM, 0);
    if (socketFd < 0)
    {
        print_errors("socket");
    }
    server = gethostbyname(hostname);
    if (server == NULL)
    {
        print_errors("host");
    } //check if hostname is retrieved
    memset((char *)&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&address.sin_addr.s_addr, server->h_length);
    address.sin_port = htons(portNum);
    if (connect(socketFd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        print_errors("connection");
    }
    dprintf(socketFd, "ID=%s\n", idNum);
    dprintf(fd, "ID=%s\n", idNum);

    temperatureSensor = mraa_aio_init(0);
    if (temperatureSensor == NULL)
    {
        print_errors("temp");
    }

    setupPollandTime();

    mraa_aio_close(temperatureSensor);
    close(fd);

    exit(0);
}
//button is no longer used as a method of shutdown
