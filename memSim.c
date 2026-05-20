#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include "circularQueue.h"

#define PAGE_TABLE_ENTRIES 256
#define PAGE_SIZE 256
#define FRAME_SIZE 256
#define TOTAL_MEM_SIZE 65536
//#define PHYSICAL_MEM_SZE 256 * FRAMES

int main(int argc, char *argv[]) {
	char *fileToRead; // address.txt
	int frames; // number of frames
	char *PRA; // FIFO, OPT, or LRU
	

	// Initialize Queue for TLB
	CircularQueue queue;
	CircularQueue_init(&queue, MAX_TLB);

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
	FILE *fptr;
	char buffer[255];

	fptr = fopen(fileToRead, "r");
	if (fptr == NULL) {
		fprintf(stderr, "ERROR: fopen().\n");
		exit(1);
	}
	
	// 2^8 = 256 bytes
	// Logical address is 16 bits:
	// Upper 8 bits = page
	// Lower 8 bits = offset
	while (fgets(buffer, sizeof(buffer), fptr)) {
		printf("Logical Sequence: %s", buffer);
		// 16916 = 2^14 + 2^9 + + 2^4 + 2^2
		// to decimal: 0100_0010_0001_0100	
		// Extracting the logical sequence
		
		int page = ((atoi(buffer) >> 8) & 0xFF);
		int offset = atoi(buffer) & 0xFF;
		//int offset = atoi(buffer) % 256; Another way to find offset
		printf("page: %d\n", page);
		printf("Offset: %d\n", offset);

		if (CircularQueue_get(&queue, page) > 0) {
			char *page_hit = "PAGE HIT\n";

		} else {
			// Insert into the TLB
			CircularQueue_insert(&queue, page, offset);
			printf("inserted: page %d with offset %d\n", page, offset);
		}

	}
	
	fclose(fptr);	
	return 0;
}
