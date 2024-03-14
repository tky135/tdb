# compile settings
CC = clang
CXX = clang++

# compile flags
CFLAGS = -Wall -g
CXXFLAGS = -Wall -g

# linker flags
LDFLAGS = 

all: main
main: linenoise.o main.o
	$(CXX) $(LDFLAGS) $^ -o $@

# test program
test: test.o
	$(CXX) $(LDFLAGS) $^ -o $@
	
# compile c source files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# compile c++ source files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -std=c++11 -c $< -o $@



