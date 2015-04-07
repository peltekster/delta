#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <cstring>
#include "common.h"
using namespace std;

inline int32_t getRandUInt() {
  return rand();
}

void testDelta() {
  const uint32_t size = (1 << 22) + 1;
  const uint32_t allocBytes = uint32_t(1.05 * size * sizeof(int32_t));

  int32_t* input = (int32_t*) malloc(allocBytes);
  int32_t* check = (int32_t*) malloc(allocBytes);
  uint8_t* enc = (uint8_t*) malloc(allocBytes);
  int32_t* dec = (int32_t*) malloc(allocBytes);

  for (auto i = 0; i < size; ++i) {
    input[i] = rand();
  }

  memcpy(check, input, allocBytes);

  clock_t start_enc = clock();
  int32_t bytes = encodeDelta(input, size, enc);

  clock_t lap = clock();
  int32_t elem = decodeDelta(enc, bytes, dec);
  clock_t end_dec = clock();

  printf("Efficiency: %.2lf bits/int\n", (double) 8. * bytes / size);
  printf("Encode speed: %.2lf Mint/s\n",
      (size / 1000000.) / (double(lap - start_enc) / CLOCKS_PER_SEC));
  printf("Decode speed: %.2lf Mint/s\n",
      (size / 1000000.) / (double(end_dec - lap) / CLOCKS_PER_SEC));

  assert(elem == size);
  for (auto i = 0; i < size; ++i) {
    assert(check[i] == dec[i]);
  }

  free(input);
  free(check);
  free(enc);
  free(dec);

  printf("Delta: OK\n");
}

void testBitpack() {
  const uint32_t size = MINIBLOCK_SIZE;
  const uint32_t allocBytes = (size + 20) * sizeof(uint32_t);

  uint32_t* bla = (uint32_t*) malloc(allocBytes);
  uint8_t* enc = (uint8_t*) malloc(allocBytes);
  uint32_t* dec = (uint32_t*) malloc(allocBytes);

  for (auto i = 0; i < size; ++i) {
    bla[i] = (uint32_t) rand();
    assert((bla[i] & 0x80000000) == 0);
  }

  int32_t bytes = bitPack(bla, size, enc);
  int32_t bytes_read = bitUnpack(enc, size, dec);

  assert(bytes_read == bytes);
  for (auto i = 0; i < size; ++i) {
    if (bla[i] != dec[i]) {
      printf("--> %u   %u->%u\n", i, bla[i], dec[i]);
      assert(false);
    }
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
    uint32_t bla = rand();
    int32_t bytes, read;
    bytes = writeVarInt(bla, enc);
    read = readVarInt(enc, &dec);
    assert(read == bytes);
    assert(dec == bla);
  }

  free(enc);

  printf("VarInt: OK\n");
}

void testZigZagInt() {
  const uint32_t tries = 1000;
  const uint32_t allocBytes = 4096;

  uint8_t* enc = (uint8_t*) malloc(allocBytes);
  int32_t dec;

  for (auto i = 0; i < tries; ++i) {
    int32_t bla = rand(), bytes, read;
    bytes = writeZigZagInt(bla, enc);
    read = readZigZagInt(enc, &dec);
    assert(read == bytes);
    assert(dec == bla);
  }

  free(enc);

  printf("ZigZagInt: OK\n");
}

int main() {
  //srand(time(0));

  while (1) {
    unsigned long seed = clock();
    srand(seed);
    printf("seed = %lu\n", seed);

    testVarInt();
    testZigZagInt();
    testBitpack();
    testDelta();
  }

  return 0;
}
