CC = gcc
CFLAGS = -O2 -Wall

uneven: uneven.o
	$(CC) $(CFLAGS) -o uneven uneven.o -lpulse

uneven.o:
	$(CC) $(CFLAGS) -c uneven.c

.PHONY: clean

clean:
	$(RM) *.o uneven
