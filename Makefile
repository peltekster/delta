# CXX = /usr/local/bin/g++-4.9
CXX = g++-4.9
LD = $(CXX)
CXXFLAGS = -O2 -std=c++11 -DBITPACK=64 -DNDEBUG
NASM = /usr/local/bin/nasm

all: delta

delta: common.o delta.o bmi2.o

bmi2.o: bmi2.s
	$(NASM) -o bmi2.o -f macho64 bmi2.s

clean:
	-rm -f delta *.o

