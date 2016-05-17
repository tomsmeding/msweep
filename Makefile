CC = gcc
CFLAGS = -Wall -Wextra -O2 -std=c99
BIN = msweep

src_files = $(wildcard *.c)

.PHONY: all clean remake

all: $(BIN)

clean:
	rm -f $(BIN)

remake: clean all


$(BIN): $(src_files)
	$(CC) $(CFLAGS) -o $@ $^
