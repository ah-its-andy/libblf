CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -g
LDFLAGS =

TARGETS = test_blf
OBJS = blf.o test_blf.o

all: $(TARGETS)

test_blf: blf.o test_blf.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c blf.h
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(TARGETS) $(OBJS)

.PHONY: all clean
