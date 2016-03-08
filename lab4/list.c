#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <math.h>
#include <getopt.h>
#include <pthread.h>
#include <string.h>

#include "SortedList.h"

SortedList_t list;
char** randstrings; // array to hold addresses of random strings for each element
SortedListElement_t* elements;
int num_threads;
int num_elements;
int num_iter;


int opt_yield;

void createElement(int index) {
	// Generate random string of length from 1 to 100
	srand(time(NULL));
	int string_length = rand()%101+1;
	randstrings[index] = malloc(string_length*sizeof(char));
	if (randstrings[index] == NULL) {
		fprintf(stderr, "ERROR: unable to allocate memory\n");
		exit(1);
	}
	for( int i = 0; i < string_length-1; ++i)
    randstrings[index][i] = '0' + rand()%72; // starting on '0', ending on '}'
  // append null byte
  randstrings[index][string_length-1] = '\0';

  // Save as element
  elements[index].key = randstrings[index];
}

// Wrapper function for each thread to execute

void listOps(void *arg) {
	//fprintf(stderr, "enter listOps\n");
	int i = *(int *)arg;
	fprintf(stderr, "%s\n", );
	free(arg);
	SortedListElement_t* e;
	int ret;
	fprintf(stderr, "free arg pointer\n");

  for (int j = i; j < i+num_iter; j++) {
  	SortedList_insert(&list, &elements[j]);
  }
  fprintf(stderr, "finished insert\n");
  int length = SortedList_length(&list);
  fprintf(stderr, "got length\n");
  for (int j = i; j < i+num_iter; j++) {
  	e = SortedList_lookup(&list, randstrings[j]);
  	if (e == NULL) {
  		fprintf(stderr, "ERROR: couldn't find added element\n");
  		exit(1);
  	}
  	ret = SortedList_delete(e);
  	if (ret != 0) {
  		fprintf(stderr, "ERROR: corrupt pointers for delete\n");
  		exit(1);
  	}
  }
  fprintf(stderr, "finished lookup & delete\n");
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
	num_iter = 1;
	num_threads = 1;
	char sync = 'n';
	int operations;

	int i; // iterator
	int ret; // return value

	// Initialize the mutex lock
	//pthread_mutex_init(&lock, NULL);

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
        {"yield",      optional_argument,  0,  'y' }, 
        {"sync",      optional_argument,  0,  's' }, 
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
	    
	    case 'y': // yield
	    	if (optarg != NULL)
	    		if (!strcmp(optarg, "i") && !strcmp(optarg, "d") && !strcmp(optarg, "s")) {
	    			fprintf(stderr, "ERROR: invalid yield option: %s\n", optarg);
	    			exit(1);
	    		}
	    		else 
	    			opt_yield = optarg[0];
	    	break;

	    case 's': // sync
	    	if (optarg != NULL) {
	    		if (!strcmp(optarg, "m") && !strcmp(optarg, "s") && !strcmp(optarg, "c")) {
	    			fprintf(stderr, "ERROR: invalid sync option: %s\n", optarg);
	    			exit(1);
	    		}
	    		else 
	    			sync = optarg[0];
	    	}
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
  num_elements = num_threads*num_iter;

  list.prev = &list;
  list.next = &list;
  list.key = NULL;

  randstrings = malloc(num_elements*sizeof(char *));
  if (randstrings == NULL) {
  	fprintf(stderr, "ERROR: unable to allocate memory\n");
		exit(1);
  }
  elements = malloc(num_elements*sizeof(SortedListElement_t));
  if (elements == NULL) {
  	fprintf(stderr, "ERROR: unable to allocate memory\n");
		exit(1);
  }

  // for (int i = 0; i < num_elements; i++) {
  // 	createElement(i);
  // 	printf("e[k].key %s\n", elements[i].key);
  // }

  // for (int k = 0; k < num_elements; k++) {
  // 	SortedList_insert(&list, &elements[k]);
  // }
  // printf("insert success\n");

  // printf("list.next.key: %s\n", list.next->key);
  // SortedListElement_t* e = SortedList_lookup(&list, randstrings[0]);
  // if (e == NULL) {
  // 	fprintf(stderr, "ERROR: lookup failed\n");
  // 	exit(1);
  // }
  // printf("lookup key: %s\n", e->key);

  // if (SortedList_length(&list) != num_elements)
  // 	fprintf(stderr, "ERROR: length failed");

  // for (int k = 0; k < num_elements; k++) {
  // 	SortedList_delete(&elements[k]);
  // }

  // e = SortedList_lookup(&list, randstrings[0]);
  // if (e != NULL) {
  // 	fprintf(stderr, "ERROR: delete failed\n");
  // }


  pthread_t* threads = malloc(num_threads*sizeof(pthread_t));

  for (i = 0; i < num_threads; i++) {
  	switch(sync) {
  		int* arg = (int*)malloc(sizeof(int));
  		if (arg == NULL) { fprintf(stderr, "ERROR: malloc error\n"); exit(1); }
  		*arg = i;
  		printf("arg: %d, i: %d\n", *arg, i);

  		case 'n': // no synchronization
  			ret = pthread_create(&threads[i], NULL, (void *) &listOps, (void *)arg);
		  	if (ret != 0) {
		  		fprintf(stderr, "ERROR: thread creation: error code is %d\n", ret);
		  		exit(1);
		  	}
		  	break;

  		case 'm': // pthread_mutex
  			// ret = pthread_create(&threads[i], NULL, (void *) &msum, (void *)arg);
		  	// if (ret != 0) {
		  	// 	fprintf(stderr, "ERROR: thread creation: error code is %d\n", ret);
		  	// 	exit(1);
		  	// }
  			// break;

	    case 's': // spinlock using __sync functions
	    // 	ret = pthread_create(&threads[i], NULL, (void *) &ssum, (void *)arg);
		  	// if (ret != 0) {
		  	// 	fprintf(stderr, "ERROR: thread creation: error code is %d\n", ret);
		  	// 	exit(1);
		  	// }
	    // 	break;

	    case 'c': // compare and swap
	    // 	ret = pthread_create(&threads[i], NULL, (void *) &csum, (void *)arg);
		  	// if (ret != 0) {
		  	// 	fprintf(stderr, "ERROR: thread creation: error code is %d\n", ret);
		  	// 	exit(1);
		  	// }
	    	break;
  	}
  }

  for (i = 0; i < num_threads; i++) {
  	ret = pthread_join(threads[i], NULL);
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

	for (int i = 0; i < num_elements; i++)
		free(randstrings[i]);
	free(randstrings);
	free(threads);
	free(elements);
	
  endTime = (long long)(end.tv_sec*pow(10, 9) + end.tv_nsec);
  startTime = (long long) (start.tv_sec*pow(10, 9) + start.tv_nsec);
  totalTime = endTime-startTime;
  operations = num_threads*num_iter*50*2;
  int finalLength = SortedList_length(&list);
  

  // Print to stdout
  printf("%d threads x %d iterations x (insert + lookup/delete) x (100/2 avg len) = %d operations\n", 
  								num_threads, num_iter, operations);
  if (finalLength != 0)
  	fprintf(stderr, "ERROR: final length = %d\n", finalLength);
  printf("elapsed time: %lld ns\n", totalTime);
  printf("per operation: %lld ns\n", totalTime/operations);

  exit(0);
}