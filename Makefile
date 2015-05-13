uneven: uneven.o
	gcc -O2 -Wall -o uneven uneven.o -lpulse

.PHONY: clean

clean:
	rm -f *.o uneven
