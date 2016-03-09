#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <math.h>
#include <getopt.h>
#include <pthread.h>
#include <string.h>
#include <ctype.h>

#include "SortedList.h"

SortedList_t* lists;
char** randstrings; // array to hold addresses of random strings for each element
SortedListElement_t* elements;
int num_iter;
int num_sublists;
static pthread_mutex_t * locks;
// spinlock
static int * lock_ms;

int opt_yield;

// hash function converts key to index of the appropriate list
int hash(const char* key) {
	int val = 0;
	for (int i = 0; i < strlen(key); i++) {
		val += (int) key[i];
	}
	return val % num_sublists;
}

void createElement(int index) {
	// Generate random string of length from 1 to 100
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
	printf("Enter listOps\n");
	int i = *(int *)arg;
	free(arg);
	SortedListElement_t* e;
	int ret;
	printf("Before insert\n");
  for (int j = i*num_iter; j < (i*num_iter)+num_iter; j++) {
  	int index = hash(elements[j].key);
  	SortedList_insert(&lists[index], &elements[j]);
  }
  printf("Before length\n");
  int length = SortedList_length(lists);
  printf("Before lookup/delete\n");
  for (int j = i*num_iter; j < (i*num_iter)+num_iter; j++) {
  	int index = hash(elements[j].key);
  	e = SortedList_lookup(&lists[index], randstrings[j]);
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
  printf("Finish listOps\n");
}

void mlistOps(void *arg) {
	//fprintf(stderr, "enter listOps\n");
	int i = *(int *)arg;
	free(arg);
	SortedListElement_t* e;
	int ret;

  for (int j = i*num_iter; j < (i*num_iter)+num_iter; j++) {
  	int index = hash(elements[j].key);
  	SortedList_t list = lists[index];
  	pthread_mutex_lock(&locks[index]);
  	SortedList_insert(&list, &elements[j]);
  	pthread_mutex_unlock(&locks[index]);
  }

  for (int j = i*num_iter; j < (i*num_iter)+num_iter; j++)
  	pthread_mutex_lock(&locks[hash(elements[j].key)]);
  int length = SortedList_length(lists);
  for (int j = i*num_iter; j < (i*num_iter)+num_iter; j++)
  	pthread_mutex_unlock(&locks[hash(elements[j].key)]);

  for (int j = i*num_iter; j < (i*num_iter)+num_iter; j++) {
  	int index = hash(elements[j].key);
  	SortedList_t list = lists[index];
  	pthread_mutex_lock(&locks[index]);
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
  	pthread_mutex_unlock(&locks[index]);
  }
}

void slistOps(void *arg) {
	//fprintf(stderr, "enter listOps\n");
	int i = *(int *)arg;
	free(arg);
	SortedListElement_t* e;
	int ret;

  for (int j = i*num_iter; j < (i*num_iter)+num_iter; j++) {
  	int index = hash(elements[j].key);
  	SortedList_t list = lists[index];
  	while(__sync_lock_test_and_set(&lock_ms[index], 1));
  	SortedList_insert(&list, &elements[j]);
  	__sync_lock_release(&lock_ms[index]);
  }
  
  for (int j = i*num_iter; j < (i*num_iter)+num_iter; j++)
  	while(__sync_lock_test_and_set(&lock_ms[j], 1));
  int length = SortedList_length(lists);
  for (int j = i*num_iter; j < (i*num_iter)+num_iter; j++)
  	__sync_lock_release(&lock_ms[j]);

  for (int j = i*num_iter; j < (i*num_iter)+num_iter; j++) {
  	int index = hash(elements[j].key);
  	SortedList_t list = lists[index];
  	while(__sync_lock_test_and_set(&lock_ms[index], 1));
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
  	__sync_lock_release(&lock_ms[index]);
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
	int num_threads = 1;
	int num_elements;
	num_iter = 1;
	num_sublists = 1;
	char sync = 'n';
	int operations;

	int i; // iterator
	int ret; // return value

  // Parse and handle options
  while (1) {
    int option_index = 0;
    static struct option long_options[] = {
    	// { "name",      has_arg,         *flag, val }
        {"threads",      optional_argument,  0,  't' }, 
        {"iterations",      optional_argument,  0,  'i' },
        {"yield",      optional_argument,  0,  'y' }, 
        {"sync",      optional_argument,  0,  's' }, 
        {"lists",      optional_argument,  0,  'l' }, 
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
	    	if (optarg != NULL) {
    			if (strlen(optarg) > 3) {
    				fprintf(stderr, "ERROR: invalid yield option: %s\n", optarg);
    				exit(1);
    			}
    			for (int j = 0; j < strlen(optarg); j++) {
    				if (optarg[j] == 'i')
    					opt_yield |= INSERT_YIELD;
    				else if (optarg[j] == 'd')
							opt_yield |= DELETE_YIELD;
						else if (optarg[j] == 's')
							opt_yield |= SEARCH_YIELD;
						else {
							fprintf(stderr, "ERROR: invalid yield option: %s\n", optarg);
    					exit(1);
						}
    			}
    		}
	    	break;

	    case 's': // sync
	    	if (optarg != NULL) {
	    		if (!strcmp(optarg, "m") && !strcmp(optarg, "s")) {
	    			fprintf(stderr, "ERROR: invalid sync option: %s\n", optarg);
	    			exit(1);
	    		}
	    		else 
	    			sync = optarg[0];
	    	}
	    	break;

	    case 'l': //sub list
	    	if (optarg != NULL) {
	    		for (int k = 0; k < strlen(optarg); k++) {
	    			if (!isdigit(optarg[k])) {
	    				fprintf(stderr, "ERROR: invalid list option: %s\n", optarg);
	    				exit(1);
	    			}
	    		}
	    		num_sublists = atoi(optarg);
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


  // Create and initialize lists & elements
  num_elements = num_threads*num_iter;

  lists = malloc(num_sublists*sizeof(SortedList_t));
  if (lists == NULL) {
  	fprintf(stderr, "ERROR: unable to allocate memory\n");
		exit(1);
  }
  printf("num_sublists = %d\n", num_sublists);
  for (int k = 0; k < num_sublists; k++) {
  	printf("inside assignment loop\n");
  	lists[k].prev = &lists[k];
  	lists[k].next = &lists[k];
  	lists[k].key = NULL;
  }

  if (lists[0].next != &lists[0]) {
  	printf("failed to assign next pointer\n");
  	exit(1);
  }

  locks = malloc(num_sublists*sizeof(pthread_mutex_t));
  if (locks == NULL) {
  	fprintf(stderr, "ERROR: unable to allocate memory\n");
		exit(1);
  }

  lock_ms = malloc(num_sublists*sizeof(int));
  if (lock_ms == NULL) {
  	fprintf(stderr, "ERROR: unable to allocate memory\n");
		exit(1);
  }

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

  // Initialize the mutex locks
	for (int i = 0; i < num_sublists; i++)
		pthread_mutex_init(&locks[i], NULL);

  srand(time(NULL));
  for (int i = 0; i < num_elements; i++)
  	createElement(i);

  // Start threads
  pthread_t* threads = malloc(num_threads*sizeof(pthread_t));
  if (threads == NULL) { fprintf(stderr, "ERROR: malloc error\n"); exit(1); }

  if (clock_gettime(CLOCK_MONOTONIC, &start) != 0) {
		fprintf(stderr, "ERROR: clock_gettime\n");
		exit(1);
	}

  for (i = 0; i < num_threads; i++) {
  	int* arg = (int*)malloc(sizeof(int));
		if (arg == NULL) { fprintf(stderr, "ERROR: malloc error\n"); exit(1); }
		*arg = i;

  	switch(sync) {

  		case 'n': // no synchronization
  			ret = pthread_create(&threads[i], NULL, (void *) &listOps, (void *)arg);
		  	if (ret != 0) {
		  		fprintf(stderr, "ERROR: thread creation: error code is %d\n", ret);
		  		exit(1);
		  	}
		  	break;

  		case 'm': // pthread_mutex
  			ret = pthread_create(&threads[i], NULL, (void *) &mlistOps, (void *)arg);
		  	if (ret != 0) {
		  		fprintf(stderr, "ERROR: thread creation: error code is %d\n", ret);
		  		exit(1);
		  	}
  			break;

	    case 's': // spinlock using __sync functions
	    	ret = pthread_create(&threads[i], NULL, (void *) &slistOps, (void *)arg);
		  	if (ret != 0) {
		  		fprintf(stderr, "ERROR: thread creation: error code is %d\n", ret);
		  		exit(1);
		  	}
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
	free(lists);
	free(locks);
	free(lock_ms);
	
  endTime = (long long)(end.tv_sec*pow(10, 9) + end.tv_nsec);
  startTime = (long long) (start.tv_sec*pow(10, 9) + start.tv_nsec);
  totalTime = endTime-startTime;
  operations = num_threads*num_iter*50*2;
  int finalLength = SortedList_length(lists);
  

  // Print to stdout
  printf("%d threads x %d iterations x (insert + lookup/delete) x (100/2 avg len) = %d operations\n", 
  								num_threads, num_iter, operations);
  if (finalLength != 0)
  	fprintf(stderr, "ERROR: final length = %d\n", finalLength);
  printf("elapsed time: %lld ns\n", totalTime);
  printf("per operation: %lld ns\n", totalTime/operations);

  exit(0);
}