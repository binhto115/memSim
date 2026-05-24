#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include "memSim.h"

int tlbNext = 0; // global FIFO pointer

int addressesTranslated = 0; // Tracks number of translated addresses
int pageFaults = 0; // Tracks number of page faults
int tlbHits = 0; // Track number of tlb hits
int tlbMisses = 0; // Track number of tlb misses

int OPTQueue = 10; // Define size of queue to store future readings
int OPTCount = 0; // To keep track of size
int OPTIndex = 0;

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

// FIFO victim Function
int fifo_victim(int *fifoHead, int frames) {
	int victim = *fifoHead; 
	*fifoHead = (*fifoHead + 1) % frames;
	return victim;
}

// LRU victim Function
int lru_victim(int *lastUsedFrame, int frames) {
	int victimFrame = 0;
	for (int i = 0; i < frames; i++) {
		//printf("EVICTION2\n");
		if (lastUsedFrame[i] < lastUsedFrame[victimFrame]) {
			victimFrame = i;
		}
	}
	return victimFrame;
}

// OPT victim Function
int opt_victim(int *frameToPage, int frames, int *futurePageRead, int OPTCount, int currentIndex, int *frameAge) {
    int farthestDistance = -1;
    int victimFrame = -1;

    for (int i = 0; i < frames; i++) {
        int pageToCheck = frameToPage[i];
        int currentDistance = 9999;

        for (int j = currentIndex + 1; j < OPTCount; j++) {
            if (futurePageRead[j] == pageToCheck) {
                currentDistance = j - currentIndex;
                break;
            }
        }

        if (currentDistance > farthestDistance || (currentDistance == farthestDistance && frameAge[i] < frameAge[victimFrame])) {
            farthestDistance = currentDistance;
            victimFrame = i;
        }
    }
    return victimFrame;
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
		PRA = "fifo";

		// printf("File: %s\n", fileToRead);
		// printf("PRA: %s\n", PRA);
		// printf("\n");
	} else if (argc == 3) {
		fileToRead = argv[1];
		frames = atoi(argv[2]);
		PRA = "fifo";

		if (frames <= 0 || frames > 256) {
			fprintf(stderr, "Error: 0 < frames <= 256\n");
			exit(1);
		}

		// printf("File: %s\n", fileToRead);
		// printf("Number of frames: %d\n", frames);
		// printf("PRA: %s\n", PRA);
	} else if (argc == 4) {
		fileToRead = argv[1];
		frames = atoi(argv[2]);
		if (frames <= 0 || frames > 256) {
			fprintf(stderr, "Error: 0 < frames <= 256\n");
			exit(1);
		}

		PRA = argv[3];
		if (strcmp(PRA, "fifo") != 0 && strcmp(PRA, "lru") != 0 && strcmp(PRA, "opt") != 0) {
			fprintf(stderr, "ERROR: check your replace algorithm.\n");
			exit(1);
		}

		// printf("File: %s\n", fileToRead);
		// printf("Number of frames: %d\n", frames);
		// printf("PRA: %s\n", PRA);
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
	int lastUsedFrame[frames];

	int frameAge[256];
	for (int i = 0; i < frames; i++) frameAge[i] = 0;
	int frameAgeCounter = 0;

	int lastUsedFrameCounter = 0;	
	for (int i = 0; i < frames; i ++) {
		lastUsedFrame[i] = -1;
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
		fprintf(stderr, "ERROR: opening BACKING_STORE.bin");
		exit(1);
	}


	// Init page array size to store god knows how many sequences they gonna test
	int *futurePageRead = malloc(sizeof(int) * OPTQueue);

	// Save every sequence into an array for OPT
	if (strcmp(PRA, "opt") == 0) {
		if (futurePageRead == NULL) {
			fprintf(stderr, "ERROR: malloc failed\n");
			exit(1);
		}
		char sequence_buffer[255];
		while (fgets(sequence_buffer, sizeof(sequence_buffer), addressesP)) {
			futurePageRead[OPTCount] = (atoi(sequence_buffer) >> 8); // Store pages at current count 
			OPTCount++;

			if (OPTCount == OPTQueue) {	
				OPTQueue *=2; // Increase size by two
				futurePageRead = realloc(futurePageRead, sizeof(int) * OPTQueue);
				
				if (futurePageRead == NULL) {
					fprintf(stderr, "ERROR: realloc failed\n");
					exit(1);
    			}
			}
		}	
	}

	// for (int i = 0; i < OPTCount; i++) {
	// 	printf("Page: %d\n", futurePageRead[i]);
	// }

	char addr_buffer[255]; // Buffer to store sequence to red
	fseek(addressesP, 0, SEEK_SET); // Move pointer back to the beginning of the file
	while (fgets(addr_buffer, sizeof(addr_buffer), addressesP)) {
		int logical = atoi(addr_buffer); // Logical sequence
		int page = logical >> 8; // Page of sequence
		int offset = logical & 0xFF; // Offset of sequence
		int frame = -1; // Keep track of frame 

		int currentIndex = OPTIndex;
		OPTIndex++;	// Increment OPT index for the next check
		addressesTranslated++; // Increment the translated address count

		// 1. Check TLB
		frame = in_TLB(tlb, page); // Find frame based on page
		if (frame != -1) {
			tlbHits++;

			// LRU check
			lastUsedFrameCounter++;
			if (strcmp(PRA, "lru") == 0) {
				lastUsedFrame[frame] = lastUsedFrameCounter;
			}
		} else {
			tlbMisses++; // Increment tlb miss counter

			// 2. Check page table
			if (pageTable[page].loaded) { // if the page is loaded in the pageTable
				frame = pageTable[page].frame; // Get frame from pageTable
				tlb_insert(tlb, page, frame);
				
				// LRU check
				lastUsedFrameCounter++;	
				if (strcmp(PRA, "lru") == 0) {
					lastUsedFrame[frame] = lastUsedFrameCounter;
				}
			} else { // the page is not loaded in the pageTable
				// 3. Page fault — load from backing store
				pageFaults++; // Increment page fault counter

				if (framesUsed < frames) { // At first, frameUsed = 0, frames = 10 (or any # you input)
					// Free frames still available
					frame = framesUsed; // set frame currently used = framesUsed
					framesUsed++; // Increment frameUsed count
					//printf("here\n");
				} else {
					// No free frames — eviction!!!
					int evictedPage = -1;
					if (strcmp(PRA, "fifo") == 0) {
						frame = fifo_victim(&fifoHead, frames);
						evictedPage = frameToPage[frame];
					} else if (strcmp(PRA, "lru") == 0) {
						frame = lru_victim(lastUsedFrame, frames); 
						evictedPage = frameToPage[frame];
					} else if (strcmp(PRA, "opt") == 0) {
						frame = opt_victim(frameToPage, frames, futurePageRead, OPTCount, currentIndex, frameAge);
						evictedPage = frameToPage[frame];
					}	
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
				
				frameAge[frame] = frameAgeCounter++;


				lastUsedFrameCounter++;
				if (strcmp(PRA, "lru") == 0) { 
					lastUsedFrame[frame] = lastUsedFrameCounter;
					//printf("Frame %d, count: %d\n", frame, lastUsedFrame[frame]);
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
		//printf("\nframe: %d\n", frame);

		printf("\n");
	}

	printf("Number of Translated Addresses = %d\n", addressesTranslated);
	printf("Page Faults = %d\n", pageFaults);
	printf("Page Fault Rate = %.3f\n", (double)pageFaults / addressesTranslated);
	printf("TLB Hits = %d\n", tlbHits);
	printf("TLB Misses = %d\n", tlbMisses);
	printf("TLB Hit Rate = %.3f\n", (double)tlbHits / addressesTranslated);

	free(futurePageRead);
	fclose(addressesP);
	fclose(backingStoreP);
	return 0;
}
