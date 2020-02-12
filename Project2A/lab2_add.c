// NAME: Prithvi Kannan
// EMAIL: prithvi.kannan@gmail.com
// ID: 405110096

#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <getopt.h>
#include <signal.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <time.h>
#include <sched.h>

long long counter;
long threads;
long iterations;
int opt_yield;
char syncArg;
int lock = 0;
pthread_mutex_t mutexLock;

void cleanUp(pthread_t *thread_ids)
{
    pthread_mutex_destroy(&mutexLock);
    free(thread_ids);
    exit(0);
}

void print(long *runTime, long *timePerOperation, long *numOperations)
{
    char yieldString[7];
    char argString[6];
    switch (opt_yield)
    {
    case 1:
        sprintf(yieldString, "-yield");
        break;
    default:
        yieldString[0] = 0;
        break;
    }
    switch (syncArg)
    {
    case 'm':
        sprintf(argString, "-m");
        break;
    case 's':
        sprintf(argString, "-s");
        break;
    case 'c':
        sprintf(argString, "-c");
        break;
    default:
        sprintf(argString, "-none");
        break;
    }
    fprintf(stdout, "add%s%s,%ld,%ld,%ld,%ld,%ld,%lld\n", yieldString, argString, threads, iterations, *(numOperations), *(runTime), *(timePerOperation), counter);
}

void add(long long *pointer, long long value)
{
    long long sum = *pointer + value;
    if (opt_yield)
    {
        sched_yield();
    }
    *pointer = sum;
}

void add_mutex(long long *pointer, long long value)
{
    pthread_mutex_lock(&mutexLock);
    long long sum = *pointer + value;
    if (opt_yield)
    {
        sched_yield();
    }
    *pointer = sum;
    pthread_mutex_unlock(&mutexLock);
}

void add_spin(long long *pointer, long long value)
{
    while (__sync_lock_test_and_set(&lock, 1))
        ;
    long long sum = *pointer + value;
    if (opt_yield)
    {
        sched_yield();
    }
    *pointer = sum;
    __sync_lock_release(&lock);
}

void add_cas(long long *pointer, long long value)
{
    long long oldVal;
    do
    {
        oldVal = counter;
        if (opt_yield)
        {
            sched_yield();
        }
    } while (__sync_val_compare_and_swap(pointer, oldVal, oldVal + value) != oldVal);
}

void *make_threads(void *arg)
{
    long i;
    i = 0;
    while (i < iterations)
    {
        switch (syncArg)
        {
        case 'm':
            add_mutex(&counter, 1);
            add_mutex(&counter, -1);
            i++;
            continue;
            break;
        case 's':
            add_spin(&counter, 1);
            add_spin(&counter, -1);
            i++;
            continue;
            break;
        case 'c':
            add_cas(&counter, 1);
            add_cas(&counter, -1);
            i++;
            continue;
            break;
        }
        add(&counter, 1);
        add(&counter, -1);
        i++;
    }
    return arg;
}

int main(int argc, char *argv[])
{
    threads = 1;
    iterations = 1;
    struct option args[] = {
        {"threads", required_argument, NULL, 't'},
        {"iterations", required_argument, NULL, 'i'},
        {"yield", no_argument, NULL, 'y'},
        {"sync", required_argument, NULL, 's'},
        {0, 0, 0, 0}};

    char param;
    syncArg = 0;
    while (1)
    {
        param = getopt_long(argc, argv, "", args, NULL);
        if (param == -1)
        {
            break;
        }
        switch (param)
        {
        case 't':
            threads = atoi(optarg);
            break;
        case 'i':
            iterations = atoi(optarg);
            break;
        case 'y':
            opt_yield = 1;
            break;
        case 's':
            syncArg = optarg[0];
            break;
        default:
            fprintf(stderr, "Error: Invalid arguments\n");
            exit(1);
        }
    }

    counter = 0;

    struct timespec startTime, endTime;
    clock_gettime(CLOCK_MONOTONIC, &startTime);

    pthread_t *thread_ids = malloc(threads * sizeof(pthread_t));
    if (thread_ids == NULL)
    {
        fprintf(stderr, "Error: Unable to malloc\n");
        exit(1);
    }

    int i;
    i = 0;
    while (i < threads)
    {
        if (pthread_create(&thread_ids[i], NULL, &make_threads, NULL) != 0)
        {
            fprintf(stderr, "Error: Unable to make threads\n");
            exit(1);
        }
        i++;
    }

    i = 0;
    while (i < threads)
    {
        pthread_join(thread_ids[i], NULL);
        i++;
    }

    clock_gettime(CLOCK_MONOTONIC, &endTime);

    long numOperations = threads * iterations * 2;
    long runTime = 1000000000L * (endTime.tv_sec - startTime.tv_sec) + endTime.tv_nsec - startTime.tv_nsec;
    long timePerOperation = runTime / numOperations;

    print(&runTime, &timePerOperation, &numOperations);

    cleanUp(thread_ids);
}