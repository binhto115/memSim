#ifndef TLB_H
#define TLB_H

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
} tlb;

int tlb_init(tlb *queue, int sizeQueue);
int tlb_insert(tlb *queue, int page, int frame);
int *tlb_get(tlb *queue, int page);

#endif
