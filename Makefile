CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++14 -g
LDFLAGS =

BIN = cas

obj_files = $(patsubst %.cpp,%.o,$(wildcard *.cpp))

.PHONY: all clean remake

all: $(BIN)

clean:
	rm -f $(BIN) *.o

remake: clean all

$(BIN): $(obj_files)
	$(CXX) -o $@ $^ $(LDFLAGS)

%.o: %.cpp $(wildcard *.h)
	$(CXX) $(CXXFLAGS) -c -o $@ $<
