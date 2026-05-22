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

int addressesTranslated = 0; // Tracks number of translated addresses
int pageFaults = 0; // Tracks number of page faults
int tlbHits = 0; // Track number of tlb hits
int tlbMisses = 0; // Track number of tlb misses

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
int in_TLB(TLB_Entry *tlb, int page) {
	for (int i = 0; i < TLB_SIZE; i++) {
		if (tlb[i].valid && tlb[i].page == page) {
			return tlb[i].frame;
		}
	}
	return -1;
}

// Function to insert a page and frame into the TLB 
void tlb_insert(TLB_Entry *tlb, int page, int frame) {
    tlb[tlbNext].page  = page;
    tlb[tlbNext].frame = frame;
    tlb[tlbNext].valid = 1;
    tlbNext = (tlbNext + 1) % TLB_SIZE;
}

// Function to invaldiate a page in the TLB
void tlb_invalidate(TLB_Entry *tlb, int page) {
	for (int i = 0; i < TLB_SIZE; i++) {
		if (tlb[i].page == page) {
			tlb[i].valid = 0;
			tlb[i].page  = -1;
			tlb[i].frame = -1;
		}
	}
}

int main(int argc, char *argv[]) {
	char *fileToRead; // File of sequences to read
	int frames; // Frame to track
	char *PRA; // FIFO, LRU, or OPT

	int framesUsed = 0;  // how many frames currently occupied
	int fifoHead = 0;    // index of oldest frame (next to evict)

	// ----- Page Table -----
	// Set every frame in pageTable to -1
	PageTableEntry pageTable[PAGE_TABLE_ENTRIES];
	for (int i = 0; i < PAGE_TABLE_ENTRIES; i++) {
		pageTable[i].frame  = -1;
		pageTable[i].loaded = 0;
	}
	
	// -----  TLB -----
	// Set every page and frame in TLB to -1
	TLB_Entry tlb[TLB_SIZE];
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
	signed char physicalMemory[frames][256];

	// FIFO queue: fifoQueue[frameIndex] = which page is stored there
	int frameToPage[256];
	for (int i = 0; i < frames; i++) {
		frameToPage[i] = -1;
	}

	// LRU queue: lastUsedFrame[frameIdex] = which frame is stored there
	int lastUsedFrame[FRAME_SIZE];
	int lastUsedFrameCounter = 0;	
	for (int i = 0; i < frames; i ++) {
		lastUsedFrame[i] = 0;

	}

	// Read sequence file
	FILE *addressesP = fopen(fileToRead, "r");
	if (addressesP == NULL) {
		fprintf(stderr, "ERROR: fopen().\n");
		exit(1);
	}

	// Read BackingStore
	FILE *backingStoreP = fopen("BACKING_STORE.bin", "rb");
	if (backingStoreP == NULL) {
		perror("Error opening BACKING_STORE.bin");
		exit(1);
	}

	char addr_buffer[255]; // Buffer to store sequence to red
	while (fgets(addr_buffer, sizeof(addr_buffer), addressesP)) {
		int logical = atoi(addr_buffer); // Logical sequence
		int page = logical >> 8; // Page of sequence
		int offset = logical & 0xFF; // Offset of sequence
		int frame = -1; // Keep track of frame 

		addressesTranslated++; // Increment the translated address count

		// 1. Check TLB
		frame = in_TLB(tlb, page); // Find frame based on page
		if (frame != -1) {
			tlbHits++;
			
			// LRU check
			timeCounter++;
			if (strcmp(PRA, "LRU") == 0) {
				lastUsedFrame[frame] = lastUsedFrameCounter;
			}
		} else {
			tlbMisses++; // Increment tlb miss counter

			// 2. Check page table
			if (pageTable[page].loaded) { // if the page is loaded in the pageTable
				frame = pageTable[page].frame; // Get frame from pageTable
				tlb_insert(tlb, page, frame);
				
				// LRU check
				timeCounter++;	
				if (strcmp(PRA, "LRU") == 0) {
					lastUsedFrame[frame] = LastUsedFrameCounter;
				}
			} else { // the page is not loaded in the pageTable
				// 3. Page fault — load from backing store
				pageFaults++; // Increment page fault counter

				if (framesUsed < frames) { // At first, frameUsed = 0, frames = 10 (or any # you input)
					// Free frames still available
					frame = framesUsed; // set frame currently used = framesUsed
					framesUsed++; // Increment frameUsed count
				} else {
					// No free frames — evict oldest (FIFO)
					int evictedPage;
					if (strcmp(PRA, "FIFO") == 0) {
						frame = fifoHead;
						fifoHead = (fifoHead + 1) % frames;
						evictedPage = frameToPage[fifoHead];
					} else if (strcmp(PRA, "LRU") == 0) {
						int victim = 0;
						for (int i = 0; i < FRAME_SIZE; i++) {
							if (lastUsedFrame[i] < lastUsedFrame[victim]) {
								victim = lastUsedFrame[i];
							}
						}
						frame = victim; 
						evictedPage = frameToPage[frame];
					} // else if (strcmp(PRA, "OPT") == 0) {
					
					// }

					// Unload evicted page from page table and TLB
					pageTable[evictedPage].loaded = 0;
					pageTable[evictedPage].frame  = -1;
					tlb_invalidate(tlb, evictedPage);
				}

				// Load new page from backing store into physical memory
				fseek(backingStoreP, page * 256, SEEK_SET); // move pointer to the address of page
				fread(physicalMemory[frame], 1, 256, backingStoreP); // Copy the data at the address of page to physical memory

				// Record which page is now in this frame
				frameToPage[frame] = page;

				// Update page table
				pageTable[page].frame  = frame;
				pageTable[page].loaded = 1;

				// Update the TLB 
				tlb_insert(tlb, page, frame);
				
				lastUsedFrameCounter++;
				if (strcmp(PRA, "LUR") == 0) { 
					lastUsedFrame[frame] = lastUsedFrameCounter;
				}
			}
		}

		// Read value from physical memory
		signed char value = physicalMemory[frame][offset];
		addr_buffer[strcspn(addr_buffer, "\n")] = 0;

		//PRINTS WITH FULL PAGE

		printf("%s, %d, %d, ", addr_buffer, value, frame);
		for (int i = 0; i < 256; i++) {
			printf("%02X", (unsigned char)physicalMemory[frame][i]);
		}
		printf("\n");	
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
