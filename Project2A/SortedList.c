// NAME: Prithvi Kannan
// EMAIL: prithvi.kannan@gmail.com
// ID: 405110096

#include "SortedList.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sched.h>

int opt_yield = 0;

void SortedList_insert(SortedList_t *list, SortedListElement_t *element) {
	if (!list || !element) {
        return;
	}

	if (list->next == NULL) {
        if (opt_yield && INSERT_YIELD) {
            sched_yield();
        }

		list->next = element;
		element->prev = list;
		element->next = NULL;
        	return;
	}
	
	SortedListElement_t* temp = list->next;
	SortedListElement_t* ptr = list;

   	while (temp != NULL && strcmp(element->key, temp->key) >= 0 ){
        ptr = temp;
        temp = temp->next;
    } 

    if (opt_yield && INSERT_YIELD) {
        sched_yield();
	}
	
    if (!temp) {
        ptr->next = element;
        element->prev = ptr;
        element->next = NULL;
   	} else {
		ptr->next = element;
		temp->prev = element;
		element->next = temp;
		element->prev = ptr;
    }
}

int SortedList_delete( SortedListElement_t *element) {
	if (!element) {
        return 1;
	}

    if (opt_yield && DELETE_YIELD) {
        sched_yield();
    }

    if (element->next != NULL) {
        if (element->next->prev != element) {
            return 1;
        } else {
            element->next->prev = element->prev;
        }
    }

    if (element->prev != NULL) {
        if (element->prev->next != element) {
            return 1;
        } else {
            element->prev->next = element->next;
        }
    }

    return 0;
}

SortedListElement_t *SortedList_lookup(SortedList_t *list, const char *key) {
    if (!list) {
        return NULL;
    }

    SortedListElement_t* ptr = list->next;

    while (ptr != NULL) {
        if (opt_yield && LOOKUP_YIELD) {
            sched_yield();
        }
        if (strcmp(key, ptr->key) == 0) {
            return ptr;
        }
        ptr = ptr->next;
    }
    return NULL;
}

int SortedList_length(SortedList_t *list) {
    	if (!list) {
        	return -1;
    	}

    	int len = 0;
    	SortedListElement_t* ptr = list->next;

    	while (ptr != NULL) {
        	if (opt_yield && LOOKUP_YIELD) {
          		sched_yield();
        	}

        	ptr = ptr->next;
        	len++;
    	}
    	return len;
}
