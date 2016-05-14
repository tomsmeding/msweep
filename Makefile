CXX = gcc
CXXFLAGS = -Wall -Wextra -O2
BIN = msweep

src_files = $(wildcard *.c)

.PHONY: all clean remake

all: $(BIN)

clean:
	rm -f $(BIN)

remake: clean all


$(BIN): $(src_files)
	$(CXX) $(CXXFLAGS) -o $@ $^
