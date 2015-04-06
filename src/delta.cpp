#include <cstdio>
#include <cstdlib>
#include <ctime>
#include "common.h"
using namespace std;

inline int32_t getRandSInt() {
  //return (rand() << 1) | (rand() & 1);
  return rand() % 292113 - 142134;
}

inline int32_t getRandUInt() {
  return rand();
}

void testDelta() {
  uint32_t size = 1025;
  uint32_t allocBytes = size * sizeof(int32_t);

  int32_t* bla = (int32_t*) malloc(allocBytes);
  uint8_t* enc = (uint8_t*) malloc(allocBytes);
  int32_t* dec = (int32_t*) malloc(allocBytes);

  for (auto i = 0; i < size; ++i) {
    bla[i] = getRandSInt();
  }

  clock_t start_enc = clock();
  int32_t bytes = encodeDelta(bla, size, enc);
  clock_t lap = clock();
  int32_t elem = decodeDelta(enc, bytes, dec);
  clock_t end_dec = clock();

  printf("Efficiency: %.2lf bytes/int\n", (double) bytes / size);
  printf("Encode speed: %.2lf Mint/s\n",
      (size / 1000000.) / (double(lap - start_enc) / CLOCKS_PER_SEC));
  printf("Decode speed: %.2lf Mint/s\n",
      (size / 1000000.) / (double(end_dec - lap) / CLOCKS_PER_SEC));

  assert(elem == size);
  for (auto i = 0; i < size; ++i) {
    assert(bla[i] == dec[i]);
  }

  free(bla);
  free(enc);
  free(dec);

  printf("Delta: OK\n");
}

void testBitpack() {
  const uint32_t size = 32;
  const uint32_t allocBytes = size * sizeof(int32_t);

  uint32_t* bla = (uint32_t*) malloc(allocBytes);
  uint8_t* enc = (uint8_t*) malloc(allocBytes);
  uint32_t* dec = (uint32_t*) malloc(allocBytes);

  for (auto i = 0; i < size; ++i) {
    bla[i] = getRandUInt() % 1343;
  }

  int32_t bytes = bitPack(bla, size, enc);
  int32_t elem = bitUnpack(enc, size, dec);

  assert(elem == size);
  for (auto i = 0; i < size; ++i) {
    assert(bla[i] == dec[i]);
  }

  free(bla);
  free(enc);
  free(dec);

  printf("Bitpack: OK\n");
}

void testVarInt() {
  const uint32_t tries = 1000;
  const uint32_t allocBytes = 4096;

  uint8_t* enc = (uint8_t*) malloc(allocBytes);
  uint32_t dec;

  for (auto i = 0; i < tries; ++i) {
    uint32_t bla = getRandUInt();
    int32_t bytes, read;
    bytes = writeVarInt(bla, enc);
    read = readVarInt(enc, &dec);
    assert(read == bytes);
    assert(dec == bla);
  }

  printf("VarInt: OK\n");
}

void testZigZagInt() {
  const uint32_t tries = 1000;
  const uint32_t allocBytes = 4096;

  uint8_t* enc = (uint8_t*) malloc(allocBytes);
  int32_t dec;

  for (auto i = 0; i < tries; ++i) {
    int32_t bla = getRandSInt(), bytes, read;
    bytes = writeZigZagInt(bla, enc);
    read = readZigZagInt(enc, &dec);
    assert(read == bytes);
    assert(dec == bla);
  }

  printf("ZigZagInt: OK\n");
}

int main() {
  srand(time(0));

  testVarInt();
  testZigZagInt();
  testBitpack();
  testDelta();

  return 0;
}
