# compile settings
CC = gcc
CXX = g++

# compile flags
CFLAGS = -Wall -g
CXXFLAGS = -Wall -g

# linker flags
LDFLAGS = 

all: main example
main: linenoise.o main.o
	$(CXX) $(LDFLAGS) $^ -o $@

# example program
example: test.o
	$(CXX) $(LDFLAGS) $^ -o $@
	
# gtest
gtest: gtest-all.o gtest.o
	$(CXX) $(LDFLAGS) $^ -o $@

# compile c source files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# compile c++ source files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -std=c++11 -c $< -o $@
%.o: %.cc
	$(CXX) $(CXXFLAGS) -std=c++11 -c $< -o $@
clean:
	rm -f *.o main example gtest

