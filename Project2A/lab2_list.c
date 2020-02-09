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
#include "SortedList.h"

SortedList_t head;
SortedListElement_t* elements;

long threads;
long iterations;
int opt_yield;

char syncArg;
char insertArg;
char deleteArg;
char lookupArg;

int lock = 0;
pthread_mutex_t mutexLock;

void segfault() {
	fprintf(stderr, "Error: segfault\n");
	exit(2);
}

void cleanUp(pthread_t *thread_ids, SortedListElement_t* elements) {
    pthread_mutex_destroy(&mutexLock);
    free(thread_ids);
    free(elements);
    exit(0);
}

void print(long *runTime, long *timePerOperation, long *numOperations) {
    char yieldString[7];
    char argString[6];
    sprintf(yieldString, "-");

    if (insertArg) {
        sprintf(yieldString + strlen(yieldString), "i");
    } 
    if (deleteArg) {
        sprintf(yieldString + strlen(yieldString), "d");
    }
    if (lookupArg) {
        sprintf(yieldString + strlen(yieldString), "l");
    }
    if (!insertArg && !deleteArg && !lookupArg) {
        sprintf(yieldString + strlen(yieldString), "none");
    }

    switch (syncArg) {
    case 'm':
        sprintf(argString, "-m");
        break;
    case 's':
        sprintf(argString, "-s");
        break;
    default:
        sprintf(argString, "-none");
        break;
    }
    int numLists;
    numLists = 1;
    fprintf(stdout, "list%s%s,%ld,%ld,%d,%ld,%ld,%ld\n", yieldString, argString, threads, iterations, numLists, *(numOperations), *(runTime), *(timePerOperation));
}

void* thread_function(void *stuff) {
	SortedListElement_t* array = stuff;
    switch (syncArg) {
    case 'm':
        pthread_mutex_lock(&mutexLock);
        break;
    case 's':
        while (__sync_lock_test_and_set(&lock, 1));
        break;
    }

	long i;
    i = 0;
    while (i < iterations) {
		SortedList_insert(&head, (SortedListElement_t *) (array+i));
        i++;
	}

	long len = SortedList_length(&head);
	if (len < iterations) {
        fprintf(stderr, "Error: not all in list\n");
		exit(2);
	}

	char *curr = malloc(sizeof(char)*256);

	SortedListElement_t *ptr;
    i = 0;
    while (i < iterations) {
		strcpy(curr, (array+i)->key);
        ptr = SortedList_lookup(&head, curr);
        if (ptr == NULL) {
            fprintf(stderr, "Error: unable to lookup\n");
            exit(2);
        }

		if ((SortedList_delete(ptr)) != 0) {
            fprintf(stderr, "Error: unable to delete\n");
            exit(2);
        }
        i++;
	}

    switch (syncArg) {
    case 'm':
        pthread_mutex_unlock(&mutexLock);
        break;
    case 's':
        __sync_lock_release(&lock);
        break;
    }

    return NULL;
}

int main(int argc, char* argv[]) {
	threads = 1;
	iterations = 1;
    opt_yield = 0;
	struct option args[] = {
		{"threads", required_argument, NULL, 't'},
		{"iterations", required_argument, NULL, 'i'},
		{"yield", required_argument, NULL, 'y'},
		{"sync", required_argument, NULL, 's'},
		{0, 0, 0, 0}
	};

    char param;
    syncArg = 0;
    insertArg = 0;
    deleteArg = 0;
    lookupArg = 0;
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
            {
                int b=0;
                for (b = 0; b < (int)strlen(optarg); b++) {
                    switch (optarg[b]) {
                    case 'i':
                        opt_yield |= INSERT_YIELD;
                        insertArg = 1;
                        continue;
                    case 'd':
                        opt_yield |= DELETE_YIELD;
                        deleteArg = 1;
                        continue;
                    case 'l':
                        opt_yield |= LOOKUP_YIELD;
                        lookupArg = 1;
                        continue;
                    }
                }
            }
            break;
        case 's':
            syncArg = optarg[0];
            break;
        default:
            fprintf(stderr, "Error: Invalid arguments\n");
            exit(1);
        }
    }

    signal(SIGSEGV, segfault);

	unsigned long numElements = threads * iterations;
	elements = malloc(sizeof(SortedListElement_t) * numElements);
	if (elements == NULL) {
		fprintf(stderr, "Error: Unable to malloc\n");
		exit(1);
	}

	char** keys = malloc(iterations * threads * sizeof(char*));
	if (keys == NULL) {
		fprintf(stderr, "Error: Unable to malloc\n");
		exit(1);
	}

    long i, j;
    i = 0;
    while ((unsigned) i < numElements) {
		keys[i] = malloc(sizeof(char) * 256);
		if (keys[i] == NULL) {
            fprintf(stderr, "Error: Unable to malloc\n");
            exit(1);
		}
		for (j = 0; j < 255; j++) {
			keys[i][j] = rand() % 94 + 33;
		}
		keys[i][255] = '\0';
		(elements + i)->key = keys[i];
        i++;
	}

	if (syncArg == 'm' && pthread_mutex_init(&mutexLock, NULL) != 0) {
        fprintf(stderr, "Error: Unable to create mutuex\n");
        exit(1);
  	}

    pthread_t *thread_ids = malloc(sizeof(pthread_t) * threads);
    if (thread_ids == NULL) {
        fprintf(stderr, "Error: Unable to malloc\n");
        exit(1);
    }

    struct timespec startTime, endTime;
  	clock_gettime(CLOCK_MONOTONIC, &startTime);

    i = 0;
    while (i < threads) {
  		if (pthread_create(&thread_ids[i], NULL, &thread_function, (void *) (elements + iterations * i)) != 0) {
			fprintf(stderr, "Error: Unable to make threads\n");
			exit(1);
		}
        i++;
	}

    i = 0;
    while (i < threads) {
		pthread_join(thread_ids[i], NULL);
        i++;
	}

  	clock_gettime(CLOCK_MONOTONIC, &endTime);

    long numOperations = threads * iterations * 3;
	long runTime = 1000000000L * (endTime.tv_sec - startTime.tv_sec) + endTime.tv_nsec - startTime.tv_nsec;
	long timePerOperation = runTime / numOperations;

    print(&runTime, &timePerOperation, &numOperations);

    cleanUp(thread_ids, elements);
}