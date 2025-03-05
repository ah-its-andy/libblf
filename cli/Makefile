CC = gcc
CFLAGS = -Wall -Wextra -g
LDFLAGS = 

# Path to libblf source
LIBBLF_SRC = ../src

# Source files
BLF_SRC = $(LIBBLF_SRC)/blf.c
CLI_SRC = blf_cli.c

# Object files
BLF_OBJ = $(BLF_SRC:.c=.o)
CLI_OBJ = $(CLI_SRC:.c=.o)

# Target executable
TARGET = blf

# Default target
all: $(TARGET)

# Link the executable
$(TARGET): $(BLF_OBJ) $(CLI_OBJ)
	$(CC) $(LDFLAGS) -o $@ $^

# Compile BLF library source
$(BLF_OBJ): $(BLF_SRC)
	$(CC) $(CFLAGS) -c -o $@ $<

# Compile CLI source
$(CLI_OBJ): $(CLI_SRC)
	$(CC) $(CFLAGS) -c -o $@ $<

# Clean up
clean:
	rm -f $(BLF_OBJ) $(CLI_OBJ) $(TARGET)

# Install
install: $(TARGET)
	mkdir -p $(DESTDIR)/usr/local/bin
	cp $(TARGET) $(DESTDIR)/usr/local/bin/

.PHONY: all clean install
