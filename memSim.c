#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include "circularQueue.h"

#define PAGE_TABLE_ENTRIES 256
#define PAGE_SIZE 256
#define FRAME_SIZE 256
#define TOTAL_MEM_SIZE 65536
#define TLB_SIZE 16

int tlbNext = 0; // global FIFO pointer

int addressesTranslated = 0;
int pageFaults = 0;
int tlbHits = 0;
int tlbMisses = 0;

typedef struct {
    int frame;
    int loaded;
} PageTableEntry;

typedef struct {
	int page;
	int frame;
	int valid;
} TLBEntry;

// if in tlb, return frame number, otherwise return -1
int in_TLB(int page, TLBEntry *tlb) {
	for (int i = 0; i < TLB_SIZE; i++) {
		if (tlb[i].valid && tlb[i].page == page) {
			return tlb[i].frame;
		}
	}
	return -1;
}

void tlb_insert(TLBEntry *tlb, int page, int frame) {
    tlb[tlbNext].page  = page;
    tlb[tlbNext].frame = frame;
    tlb[tlbNext].valid = 1;
    tlbNext = (tlbNext + 1) % TLB_SIZE;
}

void tlb_invalidate(TLBEntry *tlb, int page) {
	for (int i = 0; i < TLB_SIZE; i++) {
		if (tlb[i].page == page) {
			tlb[i].valid = 0;
			tlb[i].page  = -1;
			tlb[i].frame = -1;
		}
	}
}

int main(int argc, char *argv[]) {
	char *fileToRead;
	int frames;
	char *PRA;

	int framesUsed = 0;  // how many frames currently occupied
	int fifoHead = 0;    // index of oldest frame (next to evict)

	// Initialize page table and TLB
	PageTableEntry pageTable[PAGE_TABLE_ENTRIES];
	TLBEntry tlb[TLB_SIZE];

	for (int i = 0; i < PAGE_TABLE_ENTRIES; i++) {
		pageTable[i].frame  = -1;
		pageTable[i].loaded = 0;
	}

	for (int i = 0; i < TLB_SIZE; i++) {
		tlb[i].page  = -1;
		tlb[i].frame = -1;
		tlb[i].valid = 0;
	}

	// Check for arguments
	if (argc == 2) {
		fileToRead = argv[1];
		frames = 256;
		PRA = "FIFO";

		printf("File: %s\n", fileToRead);
		printf("PRA: %s\n", PRA);
		printf("\n");
	} else if (argc == 3) {
		fileToRead = argv[1];
		frames = atoi(argv[2]);
		PRA = "FIFO";

		if (frames <= 0 || frames > 256) {
			fprintf(stderr, "Error: 0 < frames <= 256\n");
			exit(1);
		}

		printf("File: %s\n", fileToRead);
		printf("Number of frames: %d\n", frames);
		printf("PRA: %s\n", PRA);
	} else if (argc == 4) {
		fileToRead = argv[1];
		frames = atoi(argv[2]);
		PRA = argv[3];

		if (frames <= 0 || frames > 256) {
			fprintf(stderr, "Error: 0 < frames <= 256\n");
			exit(1);
		}

		printf("File: %s\n", fileToRead);
		printf("Number of frames: %d\n", frames);
		printf("PRA: %s\n", PRA);
	} else {
		fprintf(stderr, "Error: invalid number of arguments.\n");
		exit(1);
	}

	// Physical memory: frames rows, 256 bytes each
	signed char physicalMemory[256][256];

	// FIFO queue: fifoQueue[frameIndex] = which page is stored there
	int fifoQueue[256];
	for (int i = 0; i < frames; i++) fifoQueue[i] = -1;

	//char frame_buffer[256];
	char addr_buffer[255];

	FILE *addressesP = fopen(fileToRead, "r");
	if (addressesP == NULL) {
		fprintf(stderr, "ERROR: fopen().\n");
		exit(1);
	}

	FILE *backingStoreP = fopen("BACKING_STORE.bin", "rb");
	if (backingStoreP == NULL) {
		perror("Error opening BACKING_STORE.bin");
		exit(1);
	}

	while (fgets(addr_buffer, sizeof(addr_buffer), addressesP)) {
		int logical = atoi(addr_buffer);
		int page    = logical >> 8;
		int offset  = logical & 0xFF;
		int frame   = -1;

		addressesTranslated++;

		// 1. Check TLB
		frame = in_TLB(page, tlb);
		if (frame != -1) {
			tlbHits++;
		} else {
			tlbMisses++;

			// 2. Check page table
			if (pageTable[page].loaded) {
				frame = pageTable[page].frame;
				tlb_insert(tlb, page, frame);
			}
			// 3. Page fault — load from backing store
			else {
				pageFaults++;

				if (framesUsed < frames) {
					// Free frames still available
					frame = framesUsed;
					framesUsed++;
				} else {
					// No free frames — evict oldest (FIFO)
					frame = fifoHead;
					int evictedPage = fifoQueue[fifoHead];
					fifoHead = (fifoHead + 1) % frames;

					// Unload evicted page from page table and TLB
					pageTable[evictedPage].loaded = 0;
					pageTable[evictedPage].frame  = -1;
					tlb_invalidate(tlb, evictedPage);
				}

				// Load new page from backing store into physical memory
				fseek(backingStoreP, page * 256, SEEK_SET);
				fread(physicalMemory[frame], 1, 256, backingStoreP);

				// Record which page is now in this frame
				fifoQueue[frame] = page;

				// Update page table
				pageTable[page].frame  = frame;
				pageTable[page].loaded = 1;

				tlb_insert(tlb, page, frame);

			}
		}

		// Read value from physical memory
		signed char value = physicalMemory[frame][offset];
		addr_buffer[strcspn(addr_buffer, "\n")] = 0;

		/* PRINTS WITH FULL PAGE

		printf("%s, %d, %d, ", addr_buffer, value, frame);
		for (int i = 0; i < 256; i++) {
			printf("%02X", (unsigned char)physicalMemory[frame][i]);
		}
			*/
		printf("frame: %d\n", frame);
		printf("page: %d\n", page);

		printf("\n");
	}

	printf("Number of Translated Addresses = %d\n", addressesTranslated);
	printf("Page Faults = %d\n", pageFaults);
	printf("Page Fault Rate = %.3f\n", (double)pageFaults / addressesTranslated);
	printf("TLB Hits = %d\n", tlbHits);
	printf("TLB Misses = %d\n", tlbMisses);
	printf("TLB Hit Rate = %.3f\n", (double)tlbHits / addressesTranslated);

	fclose(addressesP);
	fclose(backingStoreP);
	return 0;
}