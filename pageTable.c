// ----- Circular Queue Library -----
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "pageTable.h"

// Initialize all 256 structs and their members to 0
struct PageTable pageTable[MAX_PAGE_TABLE] = {0};

int PageTable_init() {
	for (int i=0; i < MAX_PAGE_TABLE; i++) {
		pageTable[i].frame = -1;
	}

	return 0;
}

int PageTable_set(int page, int frame) {
	pageTable[page].frame = frame;
	pageTable[page].loaded = 1;
	
	return 0;
}

int PageTable_look_up(int page) {
	int frame = pageTable[page].frame;
	return frame;
	
}

