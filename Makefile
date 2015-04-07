
ifdef RELEASE
	CXXFLAGS += -O2
else
	CXXFLAGS += -g3 -O0
endif

# CXX = /usr/local/bin/g++-4.9
CXX = g++
CC = $(CXX)
CXXFLAGS += -std=c++11
LDLIBS += -lm

all: delta

delta: common.o delta.o

clean:
	-rm -f delta *.o

