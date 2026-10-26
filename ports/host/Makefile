# Host build of the shared, platform-free game code (a normal computer). `make run` builds and prints a generated world.
CC ?= gcc
CFLAGS = -O1 -g -Ishim -I../../src -w -std=gnu11
SRC = ../../src/bigmap.c hostmain.c
hostmain: $(SRC)
	$(CC) $(CFLAGS) -o $@ $(SRC)
run: hostmain
	./hostmain 1234
clean:
	rm -f hostmain
