CC=clang

CFLAGS=-std=c99 -Wall -pedantic

.PHONY: all clean

all:
	$(CC) $(CFLAGS) -o fcon *.c

clean:
	rm -f *.o fcon
