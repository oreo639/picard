#ifndef __BYTE_ORDER__
#include <sys/param.h>
#define __BYTE_ORDER__ BYTE_ORDER
#define __ORDER_LITTLE_ENDIAN__ LITTLE_ENDIAN
#define __ORDER_BIG_ENDIAN__ BIG_ENDIAN
#endif

#ifndef __llvm__
#if !defined(__GNUC__) || (__GNUC__ < 4) || (__GNUC__ == 4 && __GNUC_MINOR__ < 8)

static inline uint16_t __builtin_bswap16(uint16_t x)
{
	return ((x << 8) & 0xff00) | ((x >> 8) & 0x00ff);
}

#if defined(__GNUC__) && (__GNUC__ == 4 && __GNUC_MINOR__ < 7)
static inline uint32_t __builtin_bswap32(uint32_t x)
{
	return ((x << 24) & 0xff000000) |
	       ((x <<  8) & 0x00ff0000) |
	       ((x >>  8) & 0x0000ff00) |
	       ((x >> 24) & 0x000000ff);
}

static inline uint64_t __builtin_bswap64(uint64_t x)
{
	return (uint64_t)__builtin_bswap32(x>>32) |
	      ((uint64_t)__builtin_bswap32(x&0xFFFFFFFF) << 32);
}
#endif
#endif
#endif

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define be_u64(a) __builtin_bswap64(a)
#define be_u32(a) __builtin_bswap32(a)
#define be_u16(a) __builtin_bswap16(a)
#define le_u64(a) (a)
#define le_u32(a) (a)
#define le_u16(a) (a)
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define be_u64(a) (a)
#define be_u32(a) (a)
#define be_u16(a) (a)
#define le_u64(a) __builtin_bswap64(a)
#define le_u32(a) __builtin_bswap32(a)
#define le_u16(a) __builtin_bswap16(a)
#else
#error "What's the endianness of the platform you're targeting?"
#endif
