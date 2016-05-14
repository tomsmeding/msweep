CXX = gcc
CXXFLAGS = -O3 -Wall -Wextra
SOURCE = $(wildcard *.c)
BIN = msweep

.PHONY: install uninstall clean remake all

$(BIN): msweep.c
	$(CXX) $(CXXFLAGS) -o $@ $^

install: $(BIN)
	install $(BIN) /usr/local/bin

uninstall: $(BIN)
	rm -f /usr/local/bin/$(BIN)

clean:
	rm -f $(BIN)

remake: clean $(BIN)

all: $(BIN)
