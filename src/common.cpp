#include "common.h"
#include <climits>

inline int32_t writeVarInt(uint32_t value, uint8_t* dest) {
  uint8_t* initial = dest;
  while ((value & 0xFFFFFF80) != 0) {
    *(dest++) = ((value & 0x7F) | 0x80);
    value >>= 7;
  }
  *(dest++) = (value & 0x7F);
  return dest - initial;
}

inline int32_t readVarInt(const uint8_t* __restrict__ src, uint32_t* __restrict__ dest) {
  uint32_t value = 0, i = 0, b;
  while ((b = *(src++)) & 0x80) {
    value |= ((b & 0x7F) << i);
    i += 7;
  }
  *dest = value | (b << i);
  return i / 7 + 1;
}

inline int32_t writeZigZagInt(int32_t value, uint8_t* dest) {
  return writeVarInt((value << 1) ^ (value >> 31), dest);
}

inline int32_t readZigZagInt(const uint8_t* __restrict__ src, int32_t* __restrict__ dest) {
  int32_t raw;
  int32_t bytesRead = readVarInt(src, (uint32_t*) &raw);
  *dest = ((((raw << 31) >> 31) ^ raw) >> 1) ^ (raw & (1 << 31));
  return bytesRead;
}

inline uint32_t popcnt(uint32_t value) {
  uint32_t cnt = 0;
  while (value) {
    ++cnt;
    value >>= 1;
  }
  return cnt;
}

int32_t bitPack(const uint32_t* __restrict__ values, uint32_t count, uint8_t* __restrict__ dest) {
  uint8_t* initDest = dest;
  uint32_t max = 0;
  for (auto i = 0; i < count; ++i) {
    max = MAX(max, values[i]);
  }

  auto bits = popcnt(max);
  auto mask = (1 << bits) - 1;
  *(dest++) = (uint8_t) bits;

  uint64_t buffer = 0;
  uint32_t bufSize = 0;
  for (auto i = 0; i < count; ++i) {
    while (bufSize >= 8) {
      *(dest++) = buffer & 0xFF;
      buffer >>= 8;
      bufSize -= 8;
    }
    buffer |= ((values[i] & mask) << bufSize);
    bufSize += bits;
    assert(bufSize <= 40);
  }
  while (bufSize >= 8) {
    *(dest++) = buffer & 0xFF;
    buffer >>= 8;
    bufSize -= 8;
  }
  assert(bufSize == 0);
  return dest - initDest;
}

int32_t bitUnpack(const uint8_t* __restrict__ src, uint32_t count, uint32_t* __restrict__ values) {
  uint32_t bits = src[0];
  uint32_t mask = (1 << bits) - 1;

  uint64_t buffer = 0;
  uint32_t bufSize = 0;
  count = 1 + bits * count / 8;
  for (auto i = 1; i < count; ++i) {
    buffer |= (((uint64_t) src[i]) << bufSize);
    bufSize += 8;

    while (bufSize >= bits) {
      *(values++) = (buffer & mask);
      buffer >>= bits;
      bufSize -= bits;
    }
  }
  assert(bufSize == 0);
  return count;
}

int encodeDelta(int32_t* __restrict__ data, uint32_t count, uint8_t* __restrict__ dest) {
  uint8_t* initDest = dest;
  dest += writeZigZagInt(data[0], dest);

  assert((count-1) % BLOCK_SIZE == 0);
  assert(BLOCK_SIZE % MINIBLOCK_SIZE == 0);

  for (auto i = 1; i < count;) {
    const auto upperBound = i + MINIBLOCK_SIZE;
    int32_t minDelta = INT_MAX;
    for (auto j = i; j < upperBound; ++j) {
      minDelta = MIN(minDelta, data[j] - data[j - 1]);
    }
    for (auto j = i; j < upperBound; ++j) {
      data[j] -= minDelta;
    }
    dest += writeZigZagInt(minDelta, dest);
    dest += bitPack((uint32_t*) &data[i], MINIBLOCK_SIZE, dest);
    i = upperBound;
  }

  return dest - initDest;
}

int decodeDelta(const uint8_t* __restrict__ src, uint32_t count, int32_t* __restrict__ values) {
  const uint8_t* initSrc = src;
  int32_t* initValues = values;
  src += readZigZagInt(src, values);
  int32_t last = *values;
  ++values;

  while (src - initSrc < count) {
    int32_t minDelta;
    src += readZigZagInt(src, &minDelta);
    src += bitUnpack(src, MINIBLOCK_SIZE, (uint32_t*) values);
    for (auto i = 0; i < MINIBLOCK_SIZE; ++i) {
      last += minDelta + values[i];
      values[i] = last;
    }
    values += MINIBLOCK_SIZE;
  }
  assert(src - initSrc == count);
  return values - initValues;
}

void dummy() {
  int a = 5;
}
