CC = gcc

CFLAGS = -Wall -g -I.
LDFLAGS = -Wall -g

PROGS = memSim

OBJS = memSim.o

all: $(PROGS)

memSim: $(OBJS)
	$(CC) $(LDFLAGS) -o memSim $(OBJS)

memSim.o: memSim.c memSim.h
	$(CC) $(CFLAGS) -c memSim.c

clean:
	rm -f $(OBJS) $(PROGS) *~ TAGS

allclean: clean
	rm -f project3_submission.tar.gz

submission: memSim.c memSim.h Makefile README.txt
	tar -czf project3_submission.tar.gz memSim.c memSim.h Makefile README.txt