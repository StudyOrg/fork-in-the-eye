CC=clang

CFLAGS=-std=c99 -Wall -pedantic

.PHONY: all clean format

all:
	$(CC) $(CFLAGS) -o pa1 *.c

clean:
	rm -f *.o pa1 *.log

format:
	astyle *.c log.h pipes_util.h router.h --style=java -s4 -n
