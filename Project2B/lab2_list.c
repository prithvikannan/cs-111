// NAME: Prithvi Kannan
// EMAIL: prithvi.kannan@gmail.com
// ID: 405110096

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <time.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include "SortedList.h"

SortedListElement_t *elements;
SortedList_t *head;

long long threads = 1;
long long iterations = 1;
long long numLists = 1;

char syncArg = 0;
int insertArg;
int deleteArg;
int lookupArg;

pthread_mutex_t *mutexLocks;
int *spinLocks;

long long *threadLockTimes;

int opt_yield = 0;

void segfault()
{
	fprintf(stderr, "Error: segfault\n");
	exit(2);
}

void cleanUp(pthread_t *thread_ids, int *thread_positions, SortedListElement_t *elements, SortedList_t *head)
{
	int i;
	i = 0;
	while (i < numLists) {
		pthread_mutex_destroy(&mutexLocks[i]);
		i++;
	}
	free(mutexLocks);
	free(spinLocks);
	free(thread_ids);
	free(thread_positions);
	free(head);
	free(elements);
	exit(0);
}

void print(long long *runTime, long *timePerOperation, long long *numOperations)
{
	char yieldString[7];
	char argString[6];

	sprintf(yieldString, "-");
	if (insertArg)
	{
		sprintf(yieldString + strlen(yieldString), "i");
	}
	if (deleteArg)
	{
		sprintf(yieldString + strlen(yieldString), "d");
	}
	if (lookupArg)
	{
		sprintf(yieldString + strlen(yieldString), "l");
	}
	if (!insertArg && !deleteArg && !lookupArg)
	{
		sprintf(yieldString + strlen(yieldString), "none");
	}

	switch (syncArg)
	{
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

	fprintf(stdout, "list%s%s,%lld,%lld,%lld,%lld,%lld,%ld\n", yieldString, argString, threads, iterations, numLists, *(numOperations), *(runTime), *(timePerOperation));
}

int hash(char key) {
  return key % numLists;
}

long long calculateTime (struct timespec *end, struct timespec *start) {
	return (end->tv_sec - start->tv_sec)*1000000000 + (end->tv_nsec - start->tv_nsec);
}

void *newThreadFunction(void *position)
{
  	int threadStartPosition = *((int *)position);
  	struct timespec lockStartTime;
  	struct timespec lockEndTime;
	int i;
	i = threadStartPosition;
	while (i < threadStartPosition + iterations)
	{
		int listIndex = hash(elements[i].key[0]);
		fprintf(stdout, "index: %d\n", listIndex);
		switch (syncArg)
		{
		case 'm':
			// clock_gettime(CLOCK_MONOTONIC, &lockStartTime);
			pthread_mutex_lock(&mutexLocks[listIndex]);
			// clock_gettime(CLOCK_MONOTONIC, &lockEndTime);
			// threadLockTimes[threadStartPosition/iterations] += calculateTime(&lockStartTime, &lockEndTime);
			SortedList_insert(&head[listIndex], &elements[i]);
			pthread_mutex_unlock(&mutexLocks[listIndex]);
			break;
		case 's':
			// clock_gettime(CLOCK_MONOTONIC, &lockStartTime);
			while (__sync_lock_test_and_set(&spinLocks[listIndex], 1))
			{
				continue;
			}
			// clock_gettime(CLOCK_MONOTONIC, &lockEndTime);
			// threadLockTimes[threadStartPosition/iterations] += calculateTime(&lockStartTime, &lockEndTime);
			SortedList_insert(&head[listIndex], &elements[i]);
			__sync_lock_release(&spinLocks[listIndex]);
			break;
		default:
			SortedList_insert(&head[listIndex], &elements[i]);
			break;
		}
		i++;
	}

	int length = 0;
	int randListIndex = rand() % numLists;

	switch (syncArg)
	{
	case 'm':
		// clock_gettime(CLOCK_MONOTONIC, &lockStartTime);
		pthread_mutex_lock(&mutexLocks[randListIndex]);
		// clock_gettime(CLOCK_MONOTONIC, &lockEndTime);
		// threadLockTimes[threadStartPosition/iterations] += calculateTime(&lockStartTime, &lockEndTime);
		length = SortedList_length(&head[randListIndex]);
		pthread_mutex_unlock(&mutexLocks[randListIndex]);
		break;
	case 's':
		// clock_gettime(CLOCK_MONOTONIC, &lockStartTime);
		while (__sync_lock_test_and_set(&spinLocks[randListIndex], 1))
		{
			continue;
		}
		// clock_gettime(CLOCK_MONOTONIC, &lockEndTime);
		// threadLockTimes[threadStartPosition/iterations] += calculateTime(&lockStartTime, &lockEndTime);
		length = SortedList_length(&head[randListIndex]);
		__sync_lock_release(&spinLocks[randListIndex]);
		break;
	default:
		length = SortedList_length(&head[randListIndex]);
		break;
	}
	if (length < 0)
	{
		fprintf(stderr, "Error: length is negative\n");
		exit(2);
	}

	i = threadStartPosition;
	while (i < threadStartPosition + iterations)
	{
		int listIndex = hash(elements[i].key[0]);
		switch (syncArg)
		{
		case 'm':
			// clock_gettime(CLOCK_MONOTONIC, &lockStartTime);
			pthread_mutex_lock(&mutexLocks[listIndex]);
			// clock_gettime(CLOCK_MONOTONIC, &lockEndTime);
			// threadLockTimes[threadStartPosition/iterations] += calculateTime(&lockStartTime, &lockEndTime);
			if (SortedList_delete(SortedList_lookup(&head[listIndex], elements[i].key)))
			{
				fprintf(stderr, "Error: could not delete\n");
				exit(2);
			}
			pthread_mutex_unlock(&mutexLocks[listIndex]);
			break;
		case 's':
			// clock_gettime(CLOCK_MONOTONIC, &lockStartTime);
			while (__sync_lock_test_and_set(&spinLocks[listIndex], 1))
			{
				continue;
			}
			// clock_gettime(CLOCK_MONOTONIC, &lockEndTime);
			// threadLockTimes[threadStartPosition/iterations] += calculateTime(&lockStartTime, &lockEndTime);
			if (SortedList_delete(SortedList_lookup(&head[listIndex], elements[i].key)))
			{
				fprintf(stderr, "Error: could not delete\n");
				exit(2);
			}
			__sync_lock_release(&spinLocks[listIndex]);
			break;
		default:
			if (SortedList_delete(SortedList_lookup(&head[listIndex], elements[i].key)))
			{
				fprintf(stderr, "Error: could not delete\n");
				exit(2);
			}
			break;
		}
		i++;
	}

  return NULL;
}

int main(int argc, char **argv)
{
	signal(SIGSEGV, segfault);

	opterr = 0; 

	static struct option args[] = {
		{"threads", required_argument, 0, 't'},
		{"iterations", required_argument, 0, 'i'},
		{"yield", required_argument, 0, 'y'},
		{"sync", required_argument, 0, 's'},
		{"lists", required_argument, 0, 'l'},
		{0, 0, 0, 0}};
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
			int a;
			a = 0;
			while (a < (int)strlen(optarg)) 
			{
				switch (optarg[a])
				{
				case 'i':
					opt_yield |= INSERT_YIELD;
					insertArg = 1;
					break;
				case 'd':
					opt_yield |= DELETE_YIELD;
					deleteArg = 1;
					break;
				case 'l':
					opt_yield |= LOOKUP_YIELD;
					lookupArg = 1;
					break;
				}
				a++;
			}
			}
			break;
		case 's':
			syncArg = optarg[0];
			break;
		case 'l':
			numLists = atoi(optarg);
			break;
		default:
			fprintf(stderr, "Error: Invalid arguments\n");
			exit(1);
		}
	}

	head = malloc(sizeof(SortedList_t));
	if (!head) {
		fprintf(stderr, "Error: Unable to malloc\n");
		exit(1);
	}
	head->key = NULL;
	head->next = head;
	head->prev = head;

	int numElements = threads * iterations;
	elements = malloc(numElements * sizeof(SortedListElement_t));
	if (!elements) {
		fprintf(stderr, "Error: Unable to malloc\n");
		exit(1);
	}

	int i;
	i = 0;
	while (i < numElements)
	{
		char *key = malloc(2 * sizeof(char));
		if (!key) {
			fprintf(stderr, "Error: Unable to malloc\n");
			exit(1);
		}
		key[0] = rand() % 256 + 'A';
		key[1] = '\0';
		elements[i].key = key;
		i++;
	}

	  // initialize (if option is set) a mutex for each sublist
	mutexLocks = malloc(numLists * sizeof(pthread_mutex_t));
	if (!mutexLocks) {
		fprintf(stderr, "Error: Unable to malloc\n");
		exit(1);
	}
	if (syncArg == 'm') {
		int i;
		i = 0;
		while (i < numLists) {
			pthread_mutex_init(&mutexLocks[i], NULL);
			i++;
		}
	}

	// initialize (if option is set) a spin lock for each sublist
	spinLocks = malloc(numLists * sizeof(int));
	if (!spinLocks) {
		fprintf(stderr, "Error: Unable to malloc\n");
		exit(1);
	}
	if (syncArg == 's') {
		int i;
		i = 0;
		while (i < numLists) {
			spinLocks[i] = 0;
			i++;
		}
	}

	threadLockTimes = malloc(sizeof(long long) * threads);

	struct timespec startTime;
	clock_gettime(CLOCK_MONOTONIC, &startTime);

	pthread_t *thread_ids = malloc(threads * sizeof(pthread_t));
	int *thread_positions = malloc(threads * sizeof(int));
	if (!thread_ids || !thread_positions) {
		fprintf(stderr, "Error: Unable to malloc\n");
		exit(1);
	}

	i = 0;
	while (i < threads)
	{
		thread_positions[i] = i * iterations;
		if (pthread_create(&thread_ids[i], NULL, newThreadFunction, &thread_positions[i]))
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

	struct timespec endTime;
	clock_gettime(CLOCK_MONOTONIC, &endTime);

	int listLength = SortedList_length(head);
	if (listLength != 0)
	{
		fprintf(stderr, "Error: list length is not zero\n");
		exit(2);
	}

	long long runTime = (endTime.tv_sec - startTime.tv_sec) * 1000000000 + (endTime.tv_nsec - startTime.tv_nsec);
	long long numOperations = 3 * threads * iterations;
	long timePerOperation = runTime / numOperations;

	print(&runTime, &timePerOperation, &numOperations);
	cleanUp(thread_ids, thread_positions, elements, head);
}
