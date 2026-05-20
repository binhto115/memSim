#ifndef CIRCULAR_QUEUE_H
#define CIRCULAR_QUEUE_H

#include <stdint.h>
#include <unistd.h>
#include <sys/types.h>

#define MAX_TLB 16

// Struct for TLB
typedef struct {
	int page;
	int frame;
	int valid;
} QueueEntry;

typedef struct {
	QueueEntry *entries; // Dynamically allocated array
	int sizeQueue;		 // max queue capacity
	int countQueue;		 // current number of elementsi
	int next_replace;
} CircularQueue;

int CircularQueue_init(CircularQueue *queue, int sizeQueue);
int CircularQueue_insert(CircularQueue *queue, int page, int frame);
int *CircularQueue_get(CircularQueue *queue, int page);

#endif
