TARGET=pa1
CC=clang
CFLAGS=-std=c99 -Wall -pedantic

PACK=$(TARGET).tar.gz

.PHONY: all clean format pack

all:
	$(CC) $(CFLAGS) -o $(TARGET) *.c

clean:
	rm -f *.o $(TARGET) *.log $(PACK)

format:
	astyle *.c log.h pipes_util.h router.h --style=java -s4 -n

pack: clean format
	mkdir $(TARGET) && cp *.c *.h $(TARGET)/ && tar -zcvf $(PACK) $(TARGET)/ && rm -rf $(TARGET)/
