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

int period = 1;
char unitFlag = 'F';

struct pollfd polls[1];
int logfd;
int logFlag = 0;
int stopReports = 0;

mraa_aio_context sensor;
mraa_gpio_context button;

struct tm *getCurrentTime()
{
    time_t raw;
    time(&raw);
    return localtime(&raw);
}

double convertTemp(int sensorInput)
{
    double temp = 1023.0 / (double)sensorInput - 1.0;
    temp *= R0;
    float temperature = 1.0 / (log(temp / R0) / BETA + 1 / 298.15) - 273.15;
    if (unitFlag == 'C')
    {
        return temperature;
    }
    else
    {
        return temperature * 9 / 5 + 32;
    }
}

void handle_input(const char *input)
{
    if (!strcmp(input, "OFF"))
    {
        if (logFlag)
        {
            dprintf(logfd, "OFF\n");
        }
        struct tm *currTime = getCurrentTime();
        fprintf(stdout, "%.2d:%.2d:%.2d SHUTDOWN\n", currTime->tm_hour, currTime->tm_min, currTime->tm_sec);
        if (logFlag)
        {
            dprintf(logfd, "%.2d:%.2d:%.2d SHUTDOWN\n", currTime->tm_hour, currTime->tm_min, currTime->tm_sec);
        }
        exit(0);
    }
    else if (!strcmp(input, "START"))
    {
        stopReports = 0;
        if (logFlag)
        {
            dprintf(logfd, "START\n");
        }
    }
    else if (!strcmp(input, "STOP"))
    {
        stopReports = 1;
        if (logFlag)
        {
            dprintf(logfd, "STOP\n");
        }
    }
    else if (!strncmp(input, "SCALE=", sizeof(char) * 6))
    {
        unitFlag = input[6];
        if (logFlag && stopReports == 0)
        {
            dprintf(logfd, "SCALE=%c\n", unitFlag);
        }
    }
    else if (!strncmp(input, "PERIOD=", sizeof(char) * 7))
    {
        int newPeriod = atoi(input + 7);
        period = newPeriod;
        if (logFlag && stopReports == 0)
        {
            dprintf(logfd, "PERIOD=%d\n", period);
        }
    }
    else if (!strncmp(input, "LOG", sizeof(char) * 3))
    {
        if (logFlag)
        {
            dprintf(logfd, "%s\n", input);
        }
    }
    else
    {
        fprintf(stdout, "Error: invalid command\n");
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
    polls[0].fd = STDIN_FILENO;
    polls[0].events = POLLIN | POLLERR | POLLHUP;

    while (1)
    {
        int sensorValue = mraa_aio_read(sensor);
        double tempValue = convertTemp(sensorValue);
        if (!stopReports)
        {
            struct tm *currTime = getCurrentTime();
            fprintf(stdout, "%.2d:%.2d:%.2d %.1f\n", currTime->tm_hour, currTime->tm_min, currTime->tm_sec, tempValue);
            if (logFlag && stopReports == 0)
            {
                dprintf(logfd, "%.2d:%.2d:%.2d %.1f\n", currTime->tm_hour, currTime->tm_min, currTime->tm_sec, tempValue);
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
                if (logFlag)
                {
                    dprintf(logfd, "%.2d:%.2d:%.2d SHUTDOWN\n", currTime->tm_hour, currTime->tm_min, currTime->tm_sec);
                }
                exit(0);
            }
            int ret = poll(polls, 1, 0);
            if (ret < 0)
            {
                fprintf(stderr, "Error: failed to poll\n");
                exit(1);
            }
            if (polls[0].revents && POLLIN)
            {
                int num = read(STDIN_FILENO, commandBuff, 128);
                if (num < 0)
                {
                    fprintf(stderr, "Error: unable to read\n");
                    exit(1);
                }
                int i;
                i = 0;
                while (i < num && copyIndex < 128)
                {
                    if (commandBuff[i] == '\n')
                    {
                        handle_input((char *)&copyBuff);
                        copyIndex = 0;
                        memset(copyBuff, 0, 128);
                    }
                    else
                    {
                        copyBuff[copyIndex] = commandBuff[i];
                        copyIndex++;
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
                unitFlag = 'C';
                break;
            case 'F':
                unitFlag = 'F';
                break;
            default:
                fprintf(stderr, "Error: invalid option. Must be --scale=F or --scale=C\n");
                exit(1);
                break;
            }
            break;
        case 'l':
            logFlag = 1;
            char *logFile = optarg;
            logfd = creat(logFile, 0666);
            if (logfd < 0)
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

    sensor = mraa_aio_init(mraaFlag); // if this is dev environment use 0, for submission use 1
    if (sensor == NULL)
    {
        fprintf(stderr, "Error: Unable to create temperature sensor\n");
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

    setupPollandTime();

    mraa_aio_close(sensor);
    mraa_gpio_close(button);
    close(logfd);
    exit(0);
}
