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
//#define PHYSICAL_MEM_SZE 256 * FRAMES

typedef struct {
    int frame;
    int loaded;
} PageTableEntry;



typedef struct {
	int page;
	int frame;
	int valid;
} TLBEntry;

// if in tlb, return index, otherwise return -1
int in_TLB(int page, TLBEntry *tlb) {
	for (int i = 0; i < 16; i++) {
		if (tlb[i].page == page) {
			return i;
		}
	}
	return -1;
}

int main(int argc, char *argv[]) {
	char *fileToRead; // address.txt
	int frames; // number of frames
	char *PRA; // FIFO, OPT, or LRU

	int pageTableEntries = 0;
		
	
	// Initialize page table and TLB
	PageTableEntry pageTable[PAGE_TABLE_ENTRIES];
	TLBEntry tlb[16];

	for (int i = 0; i < PAGE_TABLE_ENTRIES; i++) {
		pageTable[i].frame = -1;
		pageTable[i].loaded = 0;
	}

	for (int i = 0; i < 16; i++) {
		tlb[i].page = -1;
		tlb[i].frame = -1;
		tlb[i].valid = 0;
	}

	// Check for arguments
	if (argc == 2) {
		fileToRead = argv[1];	
		PRA = "FIFO";

		printf("File: %s\n", fileToRead);	
		printf("PRA: %s\n", PRA);
		printf("\n");
	} else if (argc == 3) {
		fileToRead = argv[1];

		frames = atoi(argv[2]);
		PRA = "FIFO";
		
		// Check number of frames
		if (frames <= 0 || frames > 256) {
			fprintf(stderr, "Error: 0 < frames <= 256\n");
			exit(1);
		}
	
		printf("File: %s\n", fileToRead);
		printf("Number of frames: %d\n", frames);
		printf("PRA: %s,", PRA);
	} else if (argc == 4) {
		fileToRead = argv[1];
		frames = atoi(argv[2]);
		PRA = argv[3];

		// Check number of frames
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

	// Open file to read
	//int fd = open(fileToRead, "O_RDONLY");
	char frame_buffer[255];
	char addr_buffer[255];

	FILE *addressesP = fopen(fileToRead, "r");
	if (addressesP == NULL) {
		fprintf(stderr, "ERROR: fopen().\n");
		exit(1);
	}

	FILE *backingStoreP = fopen("BACKING_STORE.bin", "rb");

	if (backingStoreP == NULL) {
		perror("Error opening file");
	}
	
	// 2^8 = 256 bytes
	// Logical address is 16 bits:
	// Upper 8 bits = page
	// Lower 8 bits = offset
	while (fgets(addr_buffer, sizeof(addr_buffer), addressesP)) {
		//printf("Logical Sequence: %s", addr_buffer);
		// 16916 = 2^14 + 2^9 + + 2^4 + 2^2
		// to decimal: 0100_0010_0001_0100	
		// Extracting the logical sequence
		
		int logical = atoi(addr_buffer);
		int page = logical >> 8;
		int offset = logical & 0xFF;
		int frame = -1;
		//int offset = atoi(buffer) % 256; Another way to find offset
		printf("address: %s\n", addr_buffer);
		printf("page: %d\n", page);
		printf("Offset: %d\n", offset);

		frame = in_TLB(page, tlb);
		//if frame is not in tlb
		if (frame == -1) {
			// check page table
			if (pageTable[page].loaded == 1) {
				frame = pageTable[page].frame;
			}
			// if the frame is not in the page table yet
			else {
				pageTable[page].frame = pageTableEntries;
				pageTableEntries += 1;
				pageTable[page].loaded = 1;	
				frame = pageTable[page].frame;			
			}
		}
		fseek(backingStoreP, page * 256, SEEK_SET);
    	// Read 256 bytes into the buffer
    	fread(frame_buffer, 1, 256, backingStoreP);
		signed char value = frame_buffer[offset];
		addr_buffer[strcspn(addr_buffer, "\n")] = 0;

		printf("%s, %d, %d, ", addr_buffer, value, frame);
		for (int i = 0; i < 256; i++) {
			printf("%02X", (unsigned char)frame_buffer[i]);
		}
		printf("\n");
	}
	
	fclose(addressesP);	
	return 0;
}
