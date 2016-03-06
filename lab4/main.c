#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <math.h>
#include <getopt.h>

// Global counter for all the threads
static long long counter = 0;

// Add subroutine: has race conditions
// Adds value to *pointer
void add(long long *pointer, long long value) {
        long long sum = *pointer + value;
        *pointer = sum;
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

	if (clock_gettime(CLOCK_MONOTONIC, &start) != 0) {
		fprintf(stderr, "clock_gettime error\n");
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
	      fprintf(stderr, "Error: ?? getopt returned character code 0%o ??\n", c);
    }
  }

  // Find end time for clock
  if (clock_gettime(CLOCK_MONOTONIC, &end) != 0)
		fprintf(stderr, "clock_gettime error\n");
	
  endTime = (long long)(end.tv_sec*pow(10, 9) + end.tv_nsec);
  startTime = (long long) (start.tv_sec*pow(10, 9) + start.tv_nsec);
  totalTime = endTime-startTime;
  operations = num_threads*num_iter*2;
  

  // Print to stdout
  printf("%d threads x %d iterations x (add + subtract) = %d operations\n", 
  								num_threads, num_iter, operations);
  if (counter != 0)
  	printf("ERROR: final count = %d\n", counter);
  printf("elapsed time: ns\n", totalTime);
  printf("per operation: %d ns", totalTime/operations);

  exit(0);
}