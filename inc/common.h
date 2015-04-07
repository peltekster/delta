#ifndef COMMON_H_
#define COMMON_H_

#include <cstdint>

#define BLOCK_SIZE 128
#define MINIBLOCK_SIZE 32

#define MAX(a,b) (((a)>(b))?(a):(b))
#define MIN(a,b) (((a)<(b))?(a):(b))

#define DECL_ALIGN(type, name, bytes) type name __attribute__((aligned(bytes)))

//#include <cassert>
void dummy();
#ifdef __DEBUG__
#define assert(a) if (!(a)) dummy()
#else
#define assert(a)
#endif

int32_t writeVarInt(uint32_t value, uint8_t* dest);
int32_t readVarInt(const uint8_t* __restrict__ src, uint32_t* __restrict__ dest);

int32_t writeZigZagInt(int32_t value, uint8_t* dest);
int32_t readZigZagInt(const uint8_t* __restrict__ src, int32_t* __restrict__ dest);

int32_t bitPack(const uint32_t* __restrict__ values, uint32_t count, uint8_t* __restrict__ dest);
int32_t bitUnpack(const uint8_t* __restrict__ src, uint32_t count, uint32_t* __restrict__ values);

int encodeDelta(int32_t* __restrict__ data, uint32_t count, uint8_t* __restrict__ dest);
int decodeDelta(const uint8_t* __restrict__ src, uint32_t count, int32_t* __restrict__ values);

#endif /* COMMON_H_ */
