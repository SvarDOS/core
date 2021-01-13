CFLAGS = -O3 -std=gnu89 -Wall -Wextra -pedantic

all: buildidx

buildidx: buildidx.c
	gcc buildidx.c -o buildidx $(CFLAGS)

clean:
	rm -f buildidx *.o
