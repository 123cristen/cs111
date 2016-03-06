#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <math.h>
#include <getopt.h>
#include <pthread.h>

// Global counter for all the threads
static long long counter = 0;

// Add subroutine: has race conditions
// Adds value to *pointer
void add(long long *pointer, long long value) {
        long long sum = *pointer + value;
        *pointer = sum;
}

// Wrapper function for each thread to execute
// adds 1 to counter n times
// subtracts 1 from counter n times
void sum(void *arg) {
	int n = *(int *)arg;
	for (int i = 0; i < n; ++i) {
		add(&counter, 1);
	}
	for (int i = 0; i < n; ++i) {
		add(&counter, -1);
	}
}

int main(int argc, char **argv) {
	// Declare time structures for holding time
	struct timespec start;
	struct timespec end;
	long long startTime;
	long long endTime;
	long long totalTime;

	// Number of iterations and threads
		// Can be reset using command line options
	int num_iter = 1;
	int num_threads = 1;
	int operations;
	int i; // iterator
	int ret; // return value

	if (clock_gettime(CLOCK_MONOTONIC, &start) != 0) {
		fprintf(stderr, "ERROR: clock_gettime\n");
		exit(1);
	}

  // Parse and handle options
  while (1) {
    int option_index = 0;
    static struct option long_options[] = {
    	// { "name",      has_arg,         *flag, val }
        {"threads",      optional_argument,  0,  't' }, 
        {"iterations",      optional_argument,  0,  'i' }, 
        {0,             0,                  0,   0  } // error handling
    };

    // get the next option
    int c = getopt_long(argc, argv, "", long_options, &option_index);
    
    // break when there are no further options to parse
    if (c == -1)
      break;

    switch (c) {
	    case 't': // threads
	    	if (optarg != NULL) // single thread
	    		num_threads = atoi(optarg);
	    	break;

	    case 'i': // iterations
	    	if (optarg != NULL) // single thread
	    		num_iter = atoi(optarg);
	    	break;

	    case '?': // ? returns when doesn't recognize option character
	    default:
	      fprintf(stderr, "ERROR: getopt returned character code 0%o \n", c);
	      exit(1);
    }
  }

  if (num_threads < 1) {
  	fprintf(stderr, "ERROR: number of threads must be positive\n");
  	exit(1);
  }
  if (num_iter < 1) {
  	fprintf(stderr, "ERROR: number of iterations must be positive\n");
  	exit(1);
  }

  pthread_t* threads = malloc(num_threads*sizeof(pthread_t));

  for (i = 0; i < num_threads; i++) {
  	ret = pthread_create(&threads[i], NULL, (void *) &sum, (void *)&num_iter);
  	if (ret != 0) {
  		fprintf(stderr, "ERROR: thread creation: error code is %d\n", ret);
  		exit(1);
  	}
  }

  for (i = 0; i < num_threads; i++) {
  	ret = pthread_join(&threads[i], NULL);
  	if (ret != 0) {
  		fprintf(stderr, "ERROR: joining threads: error code is %d\n", ret);
  		exit(1);
  	}
  }



  // Find end time for clock
  if (clock_gettime(CLOCK_MONOTONIC, &end) != 0) {
		fprintf(stderr, "ERROR: clock_gettime\n");
		exit(1);
	}
	
  endTime = (long long)(end.tv_sec*pow(10, 9) + end.tv_nsec);
  startTime = (long long) (start.tv_sec*pow(10, 9) + start.tv_nsec);
  totalTime = endTime-startTime;
  operations = num_threads*num_iter*2;
  

  // Print to stdout
  printf("%d threads x %d iterations x (add + subtract) = %d operations\n", 
  								num_threads, num_iter, operations);
  if (counter != 0)
  	fprintf(stderr, "ERROR: final count = %d\n", counter);
  printf("elapsed time: %lld ns\n", totalTime);
  printf("per operation: %lld ns\n", totalTime/operations);

  exit(0);
}