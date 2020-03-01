// NAME: Prithvi Kannan
// EMAIL: prithvi.kannan@gmail.com
// ID: 405110096

#define _POSIX_C_SOURCE 200809 // to get rid of dprintf warnings

#include <unistd.h>
#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <poll.h>
#include <time.h>
#include <sys/time.h>
#include <ctype.h>
#include <mraa.h>
#include <mraa/aio.h>
#include "fcntl.h" 

#if DEV
const int mraaFlag = 0;
#else
const int mraaFlag = 1;
#endif

const int BETA = 4275;
const int R0 = 100000;

const int BUFFER_SIZE = 128;

int period = 1;
char unit;

struct pollfd myPoll[1];
int fd;
int shouldLog = 0;

mraa_aio_context temperatureSensor;
mraa_gpio_context button;

int noReports = 0;

struct tm *getCurrentTime()
{
    time_t raw;
    time(&raw);
    return localtime(&raw);
}

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

void readInput(const char *input)
{
    if (!strcmp(input, "OFF"))
    {
        if (shouldLog)
        {
            dprintf(fd, "OFF\n");
        }
        struct tm *currTime = getCurrentTime();
        fprintf(stdout, "%.2d:%.2d:%.2d SHUTDOWN\n", currTime->tm_hour, currTime->tm_min, currTime->tm_sec);
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
    else if (!strncmp(input, "SCALE=", sizeof(char) * 6))
    {
        unit = input[6];
        if (shouldLog)
        {
            if (noReports == 0)
            {
                dprintf(fd, "SCALE=%c\n", unit);
            }
        }
    }
    else if (!strncmp(input, "PERIOD=", sizeof(char) * 7))
    {
        int newPeriod = atoi(input + 7);
        period = newPeriod;
        if (shouldLog)
        {
            if (noReports == 0)
            {
                dprintf(fd, "PERIOD=%d\n", period);
            }
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
        fprintf(stdout, "Error: invalid command\n");
        exit(1);
    }
}

void pollFunction()
{
    myPoll[0].fd = STDIN_FILENO;
    myPoll[0].events = POLLIN | POLLHUP | POLLERR;

    int position = 0;

    char cmdBuf[BUFFER_SIZE];
    char inputBuf[BUFFER_SIZE];
    memset(cmdBuf, 0, BUFFER_SIZE);
    memset(inputBuf, 0, BUFFER_SIZE);

    while (1)
    {
        int temperatureSensorValue = mraa_aio_read(temperatureSensor);
        double tempValue = convertTemp(temperatureSensorValue);
        if (!noReports)
        {
            struct tm *currTime = getCurrentTime();
            fprintf(stdout, "%.2d:%.2d:%.2d %.1f\n", currTime->tm_hour, currTime->tm_min, currTime->tm_sec, tempValue);
            if (shouldLog)
            {
                if (noReports == 0)
                {
                    dprintf(fd, "%.2d:%.2d:%.2d %.1f\n", currTime->tm_hour, currTime->tm_min, currTime->tm_sec, tempValue);
                }
            }
        }
        time_t startTime, endTime;
        time(&startTime);
        time(&endTime);

        while (difftime(endTime, startTime) < period)
        {
            if (mraa_gpio_read(button))
            {
                struct tm *currTime = getCurrentTime();
                fprintf(stdout, "%.2d:%.2d:%.2d SHUTDOWN\n", currTime->tm_hour, currTime->tm_min, currTime->tm_sec);
                if (shouldLog)
                {
                    dprintf(fd, "%.2d:%.2d:%.2d SHUTDOWN\n", currTime->tm_hour, currTime->tm_min, currTime->tm_sec);
                }
                exit(0);
            }
            int ret = poll(myPoll, 1, 0);
            if (ret < 0)
            {
                fprintf(stderr, "Error: failed to poll\n");
                exit(1);
            }
            if (myPoll[0].revents && POLLIN)
            {
                int num = read(STDIN_FILENO, cmdBuf, BUFFER_SIZE);
                if (num < 0)
                {
                    fprintf(stderr, "Error: unable to read\n");
                    exit(1);
                }
                int i;
                i = 0;
                while (i < num && position < BUFFER_SIZE)
                {
                    switch (cmdBuf[i])
                    {
                    case '\n':
                        readInput((char *)&inputBuf);
                        position = 0;
                        memset(inputBuf, 0, BUFFER_SIZE);
                        break;
                    default:
                        inputBuf[position] = cmdBuf[i];
                        position = position + 1;
                        break;
                    }
                    i++;
                }
            }
            time(&endTime);
        }
    }
}

int main(int argc, char **argv)
{
    static struct option args[] = {
        {"period", required_argument, 0, 'p'},
        {"scale", required_argument, 0, 's'},
        {"log", required_argument, 0, 'l'},
        {0, 0, 0, 0}};

    int param = 0;
    unit = 'F';
    while (1)
    {
        param = getopt_long(argc, argv, "p:sl", args, NULL);
        if (param == -1)
        {
            break;
        }
        switch (param)
        {
        case 'p':
            period = (int)atoi(optarg);
            if (period == 0)
            {
                fprintf(stderr, "Error: period cannot be 0\n");
                exit(1);
            }
            break;
        case 's':
            switch (optarg[0])
            {
            case 'C':
                unit = 'C';
                break;
            case 'F':
                unit = 'F';
                break;
            default:
                fprintf(stderr, "Error: invalid option. Must be --scale=F or --scale=C\n");
                exit(1);
                break;
            }
            break;
        case 'l':
            shouldLog = 1;
            char *fileName = optarg;
            fd = creat(fileName, 0666);
            if (fd < 0)
            {
                fprintf(stderr, "Error: failed to create file\n");
                exit(1);
            }
            break;
        default:
            fprintf(stderr, "Error: Incorrect argument! correct usage is ./lab4a --period=# [--scale=tempOpt] [--log=filename]\n");
            exit(1);
            break;
        }
    }

    temperatureSensor = mraa_aio_init(mraaFlag); // if this is dev environment use 0, for submission use 1
    if (temperatureSensor == NULL)
    {
        fprintf(stderr, "Error: Unable to create temperature temperatureSensor\n");
        mraa_deinit();
        exit(1);
    }

    button = mraa_gpio_init(60);
    if (button == NULL)
    {
        fprintf(stderr, "Error: Unable to create button\n");
        mraa_deinit();
        exit(1);
    }
    mraa_gpio_dir(button, MRAA_GPIO_IN);

    pollFunction();

    mraa_aio_close(temperatureSensor);
    mraa_gpio_close(button);
    close(fd);
    exit(0);
}
