// ----- Circular Queue Library -----
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "circularQueue.h"

int CircularQueue_init(CircularQueue *queue, int size) {
	// Allocate size number of QueueEntry struct
	queue->entries = malloc(sizeof(QueueEntry) * size);
	if (!queue->entries) {
		return -1;
	}

	queue->sizeQueue = size;
	queue->countQueue = 0;
	queue->next_replace = 0;

	for (int i = 0; i < size; i++) {
		queue->entries[i].page = -1;
		queue->entries[i].frame = -1;
		queue->entries[i].valid = 0;
	}
}

int CircularQueue_insert(CircularQueue *queue, int page, int frame) {
	QueueEntry *entry = &queue->entries[queue->next_replace];
	
	entry->page = page;
	entry->frame = frame;
	entry->valid = 1;
	queue->next_replace = (queue->next_replace + 1) % MAX_TLB;
	
	if (queue->countQueue < queue->sizeQueue){
		queue->countQueue++;
	}
	return 0;
}

int *CircularQueue_get(CircularQueue *queue, int page) {
	for (int i = 0; i <= queue->countQueue; i++) {
		if (queue->entries[i].page == page) {
			return &queue->entries[i].frame;			
		}
	}
	return 0;
	
}

