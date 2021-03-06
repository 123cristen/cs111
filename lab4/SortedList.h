/*
 * SortedList (and SortedListElement)
 *
 *	A doubly linked list, kept sorted by a specified key.
 *	This structure is used for a list head, and each element
 *	of the list begins with this structure.
 *
 *	The list head is in the list, and an empty list contains
 *	only a list head.  The list head is also recognizable because
 *	it has a NULL key pointer.
 */
struct SortedListElement {
	struct SortedListElement *prev;
	struct SortedListElement *next;
	const char *key;
};
typedef struct SortedListElement SortedList_t;
typedef struct SortedListElement SortedListElement_t;

/**
 * variable to enable diagnositc calls to pthread_yield
 */
extern int opt_yield;
extern int num_sublists;
#define	INSERT_YIELD	0x01	// yield in insert critical section
#define	DELETE_YIELD	0x02	// yield in delete critical section
#define	SEARCH_YIELD	0x04	// yield in lookup/length critical section

/**
 * SortedList_insert ... insert an element into a sorted list
 *
 *	The specified element will be inserted in to
 *	the specified list, which will be kept sorted
 *	in ascending order based on associated keys
 *
 * @param SortedList_t *list ... header for the list
 * @param SortedListElement_t *element ... element to be added to the list
 *
 * Note: if (opt_yield & INSERT_YIELD)
 *		call pthread_yield in middle of critical section
 */
void SortedList_insert(SortedList_t *list, SortedListElement_t *element) {
	SortedListElement_t * p = list;
	SortedListElement_t * n = list->next;
	while(n != list) { // condition will also fail if the list is empty, conveniently
		if (strcmp(element->key, n->key) <= 0)
			break;
		n = n->next;
	}
	if (opt_yield & INSERT_YIELD)
		pthread_yield();
	p = n->prev;
	element->prev = p;
	element->next = n;
	p->next = element;
	n->prev = element;
}

/**
 * SortedList_delete ... remove an element from a sorted list
 *
 *	The specified element will be removed from whatever
 *	list it is currently in.
 *
 *	Before doing the deletion, we check to make sure that
 *	next->prev and prev->next both point to this node
 *
 * @param SortedListElement_t *element ... element to be removed
 *
 * @return 0: element deleted successfully, 1: corrtuped prev/next pointers
 *
 * Note: if (opt_yield & DELETE_YIELD)
 *		call pthread_yield in middle of critical section
 */
int SortedList_delete( SortedListElement_t *element) {
	SortedListElement_t * n = element->next;
	SortedListElement_t * p = element->prev;
	if (opt_yield & DELETE_YIELD)
		pthread_yield();

	if (n->prev != element)
		return 1;
	if (p->next != element)
		return 1;
	n->prev = p;
	p->next = n;
	element->next = NULL;
	element->prev = NULL;
	return 0;
}

/**
 * SortedList_lookup ... search sorted list for a key
 *
 *	The specified list will be searched for an
 *	element with the specified key.
 *
 * @param SortedList_t *list ... header for the list
 * @param const char * key ... the desired key
 *
 * @return pointer to matching element, or NULL if none is found
 *
 * Note: if (opt_yield & SEARCH_YIELD)
 *		call pthread_yield in middle of critical section
 */
SortedListElement_t *SortedList_lookup(SortedList_t *list, const char *key) {
	SortedListElement_t * n = list->next;
	if (opt_yield & SEARCH_YIELD)
		pthread_yield();
	while(n != list) { // condition will also fail if the list is empty, conveniently
		if (strcmp(key, n->key) == 0)
			return n;
		n = n->next;
	}
	return NULL;
}

/**
 * SortedList_length ... count elements in a sorted list
 *	While enumeratign list, it checks all prev/next pointers
 *
 * @param SortedList_t *list ... header for the list
 *
 * @return int number of elements in list (excluding head)
 *	   -1 if the list is corrupted
 *
 * Note: if (opt_yield & SEARCH_YIELD)
 *		call pthread_yield in middle of critical section
 */
int SortedList_length(SortedList_t *lists) {
	int totalLength = 0;
	for (int i = 0; i < num_sublists; i++) {
		SortedList_t * list = &lists[i];
		int length = 0;
		SortedListElement_t * n = list->next;
		if (opt_yield & SEARCH_YIELD)
			pthread_yield();
		while(n != list) { // condition will also fail if the list is empty, conveniently
			length++;
			n = n->next;
		}
		totalLength += length;
	}
	return totalLength;
}

