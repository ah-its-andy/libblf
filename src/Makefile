CC = clang
CFLAGS = -Wall -Wextra -std=c99 -g
LDFLAGS =

TARGETS = test_blf libblf.so
OBJS = blf.o test_blf.o

all: $(TARGETS)

test_blf: blf.o test_blf.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

libblf.so: blf.o
	$(CC) -shared -fPIC -o $@ $^ $(LDFLAGS)

%.o: %.c blf.h
	$(CC) $(CFLAGS) -c -o $@ $<

blf.o: blf.c blf.h
	$(CC) $(CFLAGS) -fPIC -c -o $@ $<

clean:
	rm -f $(TARGETS) $(OBJS)

.PHONY: all clean
