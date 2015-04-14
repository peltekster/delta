OS := $(shell uname -s)

ifeq ($(OS), Linux)
	ifdef USE_LLVM
		CXX := clang
	endif
	
	CXXFLAGS += -D__LINUX__
	NASM_FLAGS += -f elf64
endif

ifeq ($(OS), Darwin)
	ifdef USE_LLVM
		CXX := g++
	else
		CXX := g++-4.9
	endif
	
	CXXFLAGS += -D__MACOS__
	NASM_FLAGS += -f macho64
endif

CC := $(CXX)

ifdef RELEASE
	CXXFLAGS += -O2 -DNDEBUG
else
	CXXFLAGS += -g3 -O0 -D__DEBUG__
endif

CXXFLAGS += -std=c++11 -DBITPACK=64
LDLIBS += -lm

all: delta

delta: common.o delta.o bmi2.o

bmi2.o: bmi2.s
	nasm -o bmi2.o $(NASM_FLAGS) bmi2.s

clean:
	-rm -f delta *.o
