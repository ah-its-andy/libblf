CC = clang
CFLAGS = -Wall -Wextra -std=c99 -g
LDFLAGS =

# Add path to libblf.so
LIB_PATH = ../src

# Include path for blf.h
INC_PATH = ../src

TARGET = blf_cli
OBJS = blf_cli.o

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ -L$(LIB_PATH) -lblf $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -I$(INC_PATH) -c -o $@ $<

install: $(TARGET)
	mkdir -p $(HOME)/bin
	cp $(TARGET) $(HOME)/bin/
	@echo "Installed blf_cli to $(HOME)/bin/"
	@echo "Make sure LD_LIBRARY_PATH includes $(LIB_PATH) when running"

clean:
	rm -f $(TARGET) $(OBJS)

.PHONY: all clean install
