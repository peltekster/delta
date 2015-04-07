# CXX = /usr/local/bin/g++-4.9
CXX = g++
LD = $(CXX)
CXXFLAGS = -g3 -O0 -std=c++11

all: delta

delta: common.o delta.o

clean:
	-rm -f delta *.o

