#ifndef PAGE_TABLE_H
#define PAGE_TABLE_H

#include <stdint.h>
#include <unistd.h>
#include <sys/types.h>

#define MAX_PAGE_TABLE 256

// Struct for page table
struct PageTable {
	int frame;
	int loaded;
};

// Functions to call
int PageTable_init();
int PageTable_set(int page, int frame);
int PageTable_look_up(int page);

extern struct PageTable pageTable[MAX_PAGE_TABLE];
#endif
