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

void print(long *run_time, long *time_per_op, long *operations) {
    fprintf(stdout, "add-none,%ld,%ld,%ld,%ld,%ld,%lld\n", threads, iterations, *(operations), *(run_time), *(time_per_op), counter);
}

void add(long long *pointer, long long value) {
    long long sum = *pointer + value;
    if (opt_yield) {
        sched_yield();
    }
    *pointer = sum;
}

void* make_threads(void* arg) {
	long i;	
    i = 0;
    while (i < iterations) {
        add(&counter, 1);
        add(&counter, -1);
        i++;
	}
	return arg;
}

int main(int argc, char* argv[]) {
	threads = 1;
	iterations = 1;
	struct option args[] = {
		{"threads", required_argument, NULL, 't'},
		{"iterations", required_argument, NULL, 'i'},
		{"yield", no_argument, NULL, 'y'},
		{"sync", required_argument, NULL, 's'},
		{0, 0, 0, 0}
	};

    char param;
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
			default:
				fprintf(stderr, "Bad arguments\n");
				exit(1);
        }
    }

    counter = 0;

    struct timespec startTime, endTime;
	clock_gettime(CLOCK_MONOTONIC, &startTime);

	pthread_t *thread_ids = malloc(threads * sizeof(pthread_t));
	if (thread_ids == NULL) {
		fprintf(stderr, "Could not allocate memory fot thread ids\n");
		exit(1);
	}

    int i;

    i = 0;
    while (i < threads) {
		if (pthread_create(&thread_ids[i], NULL, &make_threads, NULL) != 0) {
			fprintf(stderr, "Could not create threads\n");
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

    long operations = threads * iterations * 2;
	long run_time = 1000000000L * (endTime.tv_sec - startTime.tv_sec) + endTime.tv_nsec - startTime.tv_nsec;
	long time_per_op = run_time / operations;

    print(&run_time, &time_per_op, &operations);
    free(thread_ids);
    exit(0);

}