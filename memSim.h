#ifndef MEM_SIM_H
#define MEM_SIM_H

#include <stdint.h>
#include <unistd.h>
#include <sys/types.h>

#define PAGE_TABLE_ENTRIES 256
#define PAGE_SIZE 256
#define FRAME_SIZE 256
#define TOTAL_MEM_SIZE 65536
#define TLB_SIZE 16

// ----- PageTable Struct -----
typedef struct {
    int frame;
    int loaded;
} PageTableEntry;

// ----- TLB Struct -----
typedef struct {
	int page;
	int frame;
	int valid;
} TLB_Entry;


// ----- TLB functions -----
// If in tlb, return frame number, otherwise return -1
int in_TLB(TLB_Entry *tlb, int page);

// Function to insert a page and frame into the TLB 
void tlb_insert(TLB_Entry *tlb, int page, int frame);

// Function to invaldiate a page in the TLB
void tlb_invalidate(TLB_Entry *tlb, int page);

// FIFO victim Function
int fifo_victim(int *fifoHead, int frames);

// LRU victim Function
int lru_victim(int *lastUsedFrame, int frames);

// OPT victim Function
int opt_victim(int *frameToPage, int frames, int *futurePageRead, int OPTCount, int currentIndex);

#endif
