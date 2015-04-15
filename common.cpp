#include "common.h"
#include <climits>
#include <cstdio>
#include <immintrin.h>

#ifdef __LINUX__
#define unpack_internal_bmi2_0_8 _unpack_internal_bmi2_0_8
#define unpack_internal_bmi2_9_16 _unpack_internal_bmi2_9_16
#endif

extern "C" int32_t unpack_internal_bmi2_0_8(const uint8_t* __restrict__ src, uint32_t count,
    uint32_t* __restrict__ values, int bits);
extern "C" int32_t unpack_internal_bmi2_9_16(const uint8_t* __restrict__ src, uint32_t count,
    uint32_t* __restrict__ values, int bits);

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

int32_t bitPack_int32(const uint32_t* __restrict__ values, uint32_t count,
    uint8_t* __restrict__ dest) {
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
  uint32_t* destInt = (uint32_t*) dest;

  uint64_t buffer = 0;
  uint32_t bufSize = 0;
  for (auto i = 0; i < count; ++i) {
    buffer |= (((uint64_t) values[i] & mask) << bufSize);
    bufSize += bits;

    while (bufSize >= 32) {
      *(destInt++) = (uint32_t) (buffer & 0xFFFFFFFF);
      buffer >>= 32;
      bufSize -= 32;
    }
  }
  dest = (uint8_t*) destInt;
  while (bufSize >= 8) {
    *(dest++) = buffer & 0xFF;
    buffer >>= 8;
    bufSize -= 8;
  }
  assert(bufSize == 0);
  return dest - initDest;
}

int32_t bitUnpack_int32(const uint8_t* __restrict__ src, uint32_t count,
    uint32_t* __restrict__ values) {
  assert(count % 32 == 0);
  const uint32_t bits = src[0];
  assert(bits <= 32);
  const uint64_t mask = ((uint64_t) 1U << bits) - 1U;
  const uint32_t* srcInt = (uint32_t*) &src[1];

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

int32_t bitPack_int8(const uint32_t* __restrict__ values, uint32_t count,
    uint8_t* __restrict__ dest) {
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

int32_t bitUnpack_int8(const uint8_t* __restrict__ src, uint32_t count,
    uint32_t* __restrict__ values) {
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

int sizeCounters[33];

int32_t bitUnpack_bmi2(const uint8_t* __restrict__ src, uint32_t count,
    uint32_t* __restrict__ values) {
  assert(count % 32 == 0);
  const uint32_t bits = src[0];
  //sizeCounters[bits]++;
  assert(bits <= 32);
  if (bits > 8 /*16*/)
    return bitUnpack_int32(src, count, values);

  const uint8_t* __restrict__ saved = src;
  src++;
  /* if (bits <= 8) { */
  for (; count > 0; count -= 32) {
    src += unpack_internal_bmi2_0_8(src, 32, values, bits);
    values += 32;
  }
  /*} else {
   for (; count > 0; count -= 32) {
   src += unpack_internal_bmi2_9_16(src, 32, values, bits);
   values += 32;
   }
   }*/
  return src - saved;
}

#ifdef __LINUX__

const uint64_t masks[] = {0x0000000000000000, 0x0101010101010101, 0x0303030303030303,
  0x0707070707070707, 0x0f0f0f0f0f0f0f0f, 0x1f1f1f1f1f1f1f1f, 0x3f3f3f3f3f3f3f3f,
  0x7f7f7f7f7f7f7f7f, 0xffffffffffffffff};

int32_t bitUnpack_bmi2_inline(const uint8_t* __restrict__ src, uint32_t count,
    uint32_t* __restrict__ values) {
  assert(count % 32 == 0);
  const uint32_t bits = src[0];
  const uint64_t mask = masks[bits];
  assert(bits <= 32);

  if (bits > 8)
  return bitUnpack_int32(src, count, values);

  const uint8_t* __restrict__ saved = src;
  ++src;

  for (; count > 0; count -= 8) {
    uint64_t tmp = *(uint64_t*) src;

    __asm__ (
        "pdep %2, %1, %0 \n\t"
        : "=r"(tmp)
        : "r"(tmp), "r"(mask)
        : );

    for (int i = 0; i < 8; ++i) {
      *(values++) = tmp & 0xFF;
      tmp >>= 8;
    }

    src += bits;
  }

  return src - saved; // 1 + bits * count / 8;
}

int32_t bitUnpack_bmi2_unroll(const uint8_t* __restrict__ src, uint32_t count,
    uint32_t* __restrict__ values) {
  assert(count % 32 == 0);
  const uint32_t bits = src[0];
  const uint64_t mask = masks[bits];
  assert(bits <= 32);

  if (bits > 8)
  return bitUnpack_int32(src, count, values);

  const uint8_t* __restrict__ saved = src;
  ++src;

  for (; count > 0; count -= 32) {
    uint64_t tmp0 = *(uint64_t*) (src + 0 * bits);
    uint64_t tmp1 = *(uint64_t*) (src + 1 * bits);
    uint64_t tmp2 = *(uint64_t*) (src + 2 * bits);
    uint64_t tmp3 = *(uint64_t*) (src + 3 * bits);

    __asm__ ( "pdep %2, %1, %0" : "=r"(tmp0) : "r"(tmp0), "r"(mask) : );
    __asm__ ( "pdep %2, %1, %0" : "=r"(tmp1) : "r"(tmp1), "r"(mask) : );
    __asm__ ( "pdep %2, %1, %0" : "=r"(tmp2) : "r"(tmp2), "r"(mask) : );
    __asm__ ( "pdep %2, %1, %0" : "=r"(tmp3) : "r"(tmp3), "r"(mask) : );

    for (int i = 0; i < 8; ++i) {
      values[0 + i] = tmp0 & 0xFF;
      values[8 + i] = tmp1 & 0xFF;
      values[16 + i] = tmp2 & 0xFF;
      values[24 + i] = tmp3 & 0xFF;

      tmp0 >>= 8;
      tmp1 >>= 8;
      tmp2 >>= 8;
      tmp3 >>= 8;
    }

    values += 32;
    src += 4 * bits;
  }

  return src - saved; // 1 + bits * count / 8;
}

#endif

#include <emmintrin.h>
int32_t bitUnpack_4bits(const uint8_t* __restrict__ src, uint32_t count,
    uint32_t* __restrict__ values) {
  assert(count % 32 == 0);
  const uint32_t bits = src[0];
  assert(bits <= 32);

  if (bits != 4) {
    //printf("bits = %u\n", bits);
    //assert(bits == 4);
    return bitUnpack_int32(src, count, values);
  }

  const uint8_t* __restrict__ saved = src;
  ++src;

  const uint32_t* __restrict__ ints = (uint32_t*) src;

  for (; count > 0; count -= 8) {
    *(values++) = ((*ints)) & 15;
    *(values++) = ((*ints) >> 4) & 15;
    *(values++) = ((*ints) >> 8) & 15;
    *(values++) = ((*ints) >> 12) & 15;
    *(values++) = ((*ints) >> 16) & 15;
    *(values++) = ((*ints) >> 20) & 15;
    *(values++) = ((*ints) >> 24) & 15;
    *(values++) = ((*ints) >> 28);

    ++ints;
    src += bits;
  }

  return src - saved; // 1 + bits * count / 8;
}

const __m128i all15 = _mm_set1_epi32(15);

int32_t bitUnpack_4bits_sse(const uint8_t* __restrict__ src, uint32_t count,
    uint32_t* __restrict__ values) {
  assert(count % 32 == 0);
  const uint32_t bits = src[0];
  assert(bits <= 32);

  if (bits != 4) {
    return bitUnpack_int32(src, count, values);
  }

  const uint8_t* saved = src;
  ++src;

  __m128i* sseIn = (__m128i*) src;
  __m128i* sseOut = (__m128i*) values;

  for (; count > 0; count -= 32) {
    __m128i tmp = _mm_loadu_si128(sseIn++);

    _mm_storeu_si128(sseOut++, _mm_and_si128(tmp, all15));
    _mm_storeu_si128(sseOut++, _mm_and_si128(_mm_srli_epi32(tmp, 4), all15));
    _mm_storeu_si128(sseOut++, _mm_and_si128(_mm_srli_epi32(tmp, 8), all15));
    _mm_storeu_si128(sseOut++, _mm_and_si128(_mm_srli_epi32(tmp, 12), all15));
    _mm_storeu_si128(sseOut++, _mm_and_si128(_mm_srli_epi32(tmp, 16), all15));
    _mm_storeu_si128(sseOut++, _mm_and_si128(_mm_srli_epi32(tmp, 20), all15));
    _mm_storeu_si128(sseOut++, _mm_and_si128(_mm_srli_epi32(tmp, 24), all15));
    _mm_storeu_si128(sseOut++, _mm_srli_epi32(tmp, 28));

    src += 16; // 4*bits
  }

  return src - saved; // 1 + bits * count / 8;
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
