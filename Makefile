CC = gcc
CFLAGS = -Wall -Wextra -O2 -std=c99
BIN = msweep

.PHONY: install uninstall clean remake all

$(BIN): msweep.c
	$(CC) $(CFLAGS) -o $@ $^

install: $(BIN)
	install $(BIN) /usr/local/bin

uninstall: $(BIN)
	rm -f /usr/local/bin/$(BIN)

clean:
	rm -f $(BIN)

remake: clean $(BIN)

all: $(BIN)
