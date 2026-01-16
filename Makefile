# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -g
LDFLAGS = 

# Directories
SRC_DIR = .
OBJ_DIR = .
BIN_DIR = .

# Source files
SOURCES = main.c customAllocator.c
OBJECTS = $(SOURCES:.c=.o)
TARGET = allocator

# Default target
all: $(TARGET)

# Build the executable
$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $(TARGET) $(LDFLAGS)

# Compile source files to object files
%.o: %.c customAllocator.h
	$(CC) $(CFLAGS) -c $< -o $@

# Clean build artifacts
clean:
	rm -f $(OBJECTS) $(TARGET)

# Rebuild everything
rebuild: clean all

# Phony targets
.PHONY: all clean rebuild
