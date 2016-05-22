TARGET=pa1
PACK=$(TARGET).tar.gz
CC=clang

CFLAGS=-std=c99 -Wall -pedantic

.PHONY: all clean format pack

all:
	$(CC) $(CFLAGS) -o $(TARGET) *.c

clean:
	rm -f *.o $(TARGET) *.log $(PACK)

format:
	astyle *.c log.h pipes_util.h router.h --style=java -s4 -n

pack: clean format
	mkdir $(TARGET) && cp *.c *.h $(TARGET)/ && tar -zcvf $(PACK) $(TARGET)/ && rm -rf $(TARGET)/
