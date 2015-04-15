#ifndef COMMON_H_
#define COMMON_H_

#include <cstdint>
#include <cassert>

#define BLOCK_SIZE 128
#define MINIBLOCK_SIZE 32

#define MAX(a,b) (((a)>(b))?(a):(b))
#define MIN(a,b) (((a)<(b))?(a):(b))

#define COUNT_OF(arr) (sizeof(arr) / sizeof(arr[0]))

#define DECL_ALIGN(type, name, bytes) type name __attribute__((aligned(bytes)))

int32_t writeVarInt(uint32_t value, uint8_t* dest);
int32_t readVarInt(const uint8_t* __restrict__ src, uint32_t* __restrict__ dest);

int32_t writeZigZagInt(int32_t value, uint8_t* dest);
int32_t readZigZagInt(const uint8_t* __restrict__ src, int32_t* __restrict__ dest);

int32_t bitPack_int8(const uint32_t* __restrict__ values, uint32_t count, uint8_t* __restrict__ dest);
int32_t bitPack_int32(const uint32_t* __restrict__ values, uint32_t count, uint8_t* __restrict__ dest);

int32_t bitUnpack_int8(const uint8_t* __restrict__ src, uint32_t count, uint32_t* __restrict__ values);
int32_t bitUnpack_int32(const uint8_t* __restrict__ src, uint32_t count, uint32_t* __restrict__ values);
int32_t bitUnpack_bmi2(const uint8_t* __restrict__ src, uint32_t count, uint32_t* __restrict__ values);

int32_t bitUnpack_bmi2_inline(const uint8_t* __restrict__ src, uint32_t count,
    uint32_t* __restrict__ values);
int32_t bitUnpack_bmi2_unroll(const uint8_t* __restrict__ src, uint32_t count,
    uint32_t* __restrict__ values);
int32_t bitUnpack_4bits(const uint8_t* __restrict__ src, uint32_t count,
    uint32_t* __restrict__ values);
int32_t bitUnpack_4bits_sse(const uint8_t* __restrict__ src, uint32_t count,
    uint32_t* __restrict__ values);

#if (BITPACK == 66)
# define bitPack    bitPack_int32
# define bitUnpack  bitUnpack_4bits_sse
#endif

#if (BITPACK == 65)
# define bitPack    bitPack_int32
# define bitUnpack  bitUnpack_bmi2_unroll
#endif

#if (BITPACK == 64)
# define bitPack    bitPack_int32
# define bitUnpack  bitUnpack_bmi2
#endif

#if (BITPACK == 32)
# define bitPack    bitPack_int32
# define bitUnpack  bitUnpack_int32
#endif

#if (BITPACK == 8) || !defined(bitPack)
# define bitPack    bitPack_int8
# define bitUnpack  bitUnpack_int8
#endif


int encodeDelta(int32_t* __restrict__ data, uint32_t count, uint8_t* __restrict__ dest);
int decodeDelta(const uint8_t* __restrict__ src, uint32_t count, int32_t* __restrict__ values);

#endif /* COMMON_H_ */
