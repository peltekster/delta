#include "common.h"
#include <climits>
#include <cstdio>
#include <immintrin.h>

int32_t writeVarInt(uint32_t value, uint8_t* dest) {
  uint8_t* initial = dest;
  while ((value & 0xFFFFFF80) != 0) {
    *(dest++) = ((value & 0x7F) | 0x80);
    value >>= 7;
  }
  *(dest++) = (value & 0x7F);
  return dest - initial;
}

int32_t readVarInt(const uint8_t* __restrict__ src, uint32_t* __restrict__ dest) {
  uint32_t value = 0, i = 0, b;
  while ((b = *(src++)) & 0x80) {
    value |= ((b & 0x7F) << i);
    i += 7;
  }
  *dest = value | (b << i);
  return i / 7 + 1;
}

int32_t writeZigZagInt(int32_t value, uint8_t* dest) {
  return writeVarInt((value << 1) ^ (value >> 31), dest);
}

int32_t readZigZagInt(const uint8_t* __restrict__ src, int32_t* __restrict__ dest) {
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

int32_t bitPack_int32(const uint32_t* __restrict__ values, uint32_t count, uint8_t* __restrict__ dest) {
  assert(count % 32 == 0);

  uint8_t* initDest = dest;
  uint32_t max = 0;
  for (auto i = 0; i < count; ++i) {
    max = MAX(max, values[i]);
  }

  const uint32_t bits = popcnt(max);
  assert(bits <= 32);
  const uint64_t mask = ((uint64_t) 1U << bits) - 1U;
  *(dest++) = (uint8_t) bits;
  uint32_t* destInt = (uint32_t*)dest;

  uint64_t buffer = 0;
  uint32_t bufSize = 0;
  for (auto i = 0; i < count; ++i) {
    buffer |= (((uint64_t) values[i] & mask) << bufSize);
    bufSize += bits;

    while (bufSize >= 32) {
      *(destInt++) = (uint32_t)(buffer & 0xFFFFFFFF);
      buffer >>= 32;
      bufSize -= 32;
    }
  }
  dest = (uint8_t*)destInt;
  while (bufSize >= 8) {
    *(dest++) = buffer & 0xFF;
    buffer >>= 8;
    bufSize -= 8;
  }
  assert(bufSize == 0);
  return dest - initDest;
}

int32_t bitUnpack_int32(const uint8_t* __restrict__ src, uint32_t count, uint32_t* __restrict__ values) {
  assert(count % 32 == 0);
  const uint32_t bits = src[0];
  assert(bits <= 32);
  const uint64_t mask = ((uint64_t) 1U << bits) - 1U;
  const uint32_t* srcInt = (uint32_t*)&src[1];

  uint64_t buffer = 0;
  uint32_t bufSize = 0;
  count = bits * count / 32;
  for (auto i = 0; i < count; ++i) {
    buffer |= (((uint64_t) srcInt[i]) << bufSize);
    bufSize += 32;

    while (bufSize >= bits) {
      *(values++) = (uint32_t) (buffer & mask);
      buffer >>= bits;
      bufSize -= bits;
    }
  }
  assert(bufSize == 0);
  return 1 + count * 4;
}

int32_t bitPack_int8(const uint32_t* __restrict__ values, uint32_t count, uint8_t* __restrict__ dest) {
  assert(count % 32 == 0);

  uint8_t* initDest = dest;
  uint32_t max = 0;
  for (auto i = 0; i < count; ++i) {
    max = MAX(max, values[i]);
  }

  const uint32_t bits = popcnt(max);
  assert(bits <= 32);
  const uint64_t mask = ((uint64_t) 1U << bits) - 1U;
  *(dest++) = (uint8_t) bits;

  uint64_t buffer = 0;
  uint32_t bufSize = 0;
  for (auto i = 0; i < count; ++i) {
    buffer |= (((uint64_t) values[i] & mask) << bufSize);
    bufSize += bits;

    while (bufSize >= 8) {
      *(dest++) = (uint8_t) (buffer & 0xFF);
      buffer >>= 8U;
      bufSize -= 8U;
    }
  }
  assert(bufSize == 0);
  return dest - initDest;
}

int32_t bitUnpack_int8(const uint8_t* __restrict__ src, uint32_t count, uint32_t* __restrict__ values) {
  assert(count % 32 == 0);
  const uint32_t bits = src[0];
  assert(bits <= 32);
  const uint64_t mask = ((uint64_t) 1U << bits) - 1U;

  uint64_t buffer = 0;
  uint32_t bufSize = 0;
  count = 1U + bits * count / 8U;
  for (auto i = 1; i < count; ++i) {
    buffer |= (((uint64_t) src[i]) << bufSize);
    bufSize += 8U;

    while (bufSize >= bits) {
      *(values++) = (uint32_t) (buffer & mask);
      buffer >>= bits;
      bufSize -= bits;
    }
  }
  assert(bufSize == 0);
  return count;
}


extern "C"
  int32_t unpack_internal_bmi2(const uint8_t* __restrict__ src, uint32_t count, uint32_t* __restrict__ values, int bits);

int32_t bitUnpack_bmi2(const uint8_t* __restrict__ src, uint32_t count, uint32_t* __restrict__ values)
{
  assert(count % 32 == 0);
  const uint32_t bits = src[0];
  assert(bits <= 32);
  if (bits > 8) return bitUnpack_int32(src, count, values);


  return 1 + unpack_internal_bmi2(src + 1, count, values, bits);
}


int encodeDelta(int32_t* __restrict__ data, uint32_t count, uint8_t* __restrict__ dest) {
  uint8_t* initDest = dest;
  dest += writeZigZagInt(data[0], dest);
  int32_t last = data[0];

  assert((count-1) % BLOCK_SIZE == 0);
  assert(BLOCK_SIZE % MINIBLOCK_SIZE == 0);

  for (auto i = 1; i < count;) {
    int32_t minDelta = INT_MAX, curDelta;
    const auto upperBound = i + BLOCK_SIZE;

    for (auto j = i; j < upperBound; ++j) {
      curDelta = data[j] - last;
      last = data[j];
      data[j] = curDelta;
      minDelta = MIN(minDelta, curDelta);
    }

    for (auto j = i; j < upperBound; ++j) {
      data[j] -= minDelta;
    }

    dest += writeZigZagInt(minDelta, dest);
    for (auto j = i; j < upperBound; j += MINIBLOCK_SIZE) {
      dest += bitPack((uint32_t*) &data[j], MINIBLOCK_SIZE, dest);
    }

    i = upperBound;
    assert(i <= count);
  }

  return dest - initDest;
}

int decodeDelta(const uint8_t* __restrict__ src, uint32_t count, int32_t* __restrict__ values) {
  const uint8_t* initSrc = src;
  int32_t* initValues = values;
  int32_t last;
  src += readZigZagInt(src, &last);
  *(values++) = last;

  while (src - initSrc < count) {
    int32_t minDelta;
    src += readZigZagInt(src, &minDelta);

    for (auto br = 0; br < BLOCK_SIZE; br += MINIBLOCK_SIZE) {
      src += bitUnpack(src, MINIBLOCK_SIZE, (uint32_t*) values);
      for (auto i = 0; i < MINIBLOCK_SIZE; ++i) {
        last += minDelta + values[i];
        values[i] = last;
      }
      values += MINIBLOCK_SIZE;
    }
  }
  assert(src - initSrc == count);
  return values - initValues;
}
