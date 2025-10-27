#ifndef KRUEGER_BASE_CORE_H
#define KRUEGER_BASE_CORE_H

//////////////////////////////////
// NOTE: Standard Library Includes

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <stddef.h>
#include <float.h>
#include <math.h>
#include <string.h>
#undef min
#undef max

//////////////////////
// NOTE: Helper Macros

#if PLATFORM_WINDOWS
# define shared_function __declspec(dllexport)
#else
# define shared_function
#endif

#if COMPILER_MSVC
#define trap() __debugbreak()
#elif COMPILER_CLANG || COMPILER_GCC
#define trap() __builtin_trap()
#else
#error unknown trap intrinsic for this compiler
#endif

#define assert_always(x) do { if (!(x)) { trap(); } } while(0)
#if BUILD_DEBUG
#define assert(x) assert_always(x)
#else
#define assert(x) (void)(x)
#endif

#define invalid_path    assert(!"invalid path!")
#define not_implemented assert(!"not implemented!")

#define array_count(x) (sizeof(x)/sizeof(*(x)))
#define sqr(x) ((x)*(x))

#define KB(n) (((u64)(n))<<10)
#define MB(n) (((u64)(n))<<20)
#define GB(n) (((u64)(n))<<30)
#define TB(n) (((u64)(n))<<40)
#define thousand(n) ((n)*1000)
#define million(n)  ((n)*1000000)
#define billion(n)  ((n)*1000000000)

#define min(a, b) ((a)<(b)?(a):(b))
#define max(a, b) ((a)>(b)?(a):(b))
#define clamp_top(a, x) min(a, x);
#define clamp_bot(x, b) max(x, b);
#define clamp(a, x, b) (((x)<(a))?(a):((x)>(b))?(b):(x))

#define mem_cpy(dst, src, size)  memmove((dst), (src), (size))
#define mem_set(dst, byte, size) memset((dst), (byte), (size))
#define mem_cmp(a, b, size)      memcmp((a), (b), (size))

#define mem_zero(dst, size)  mem_set((dst), 0, (size))
#define mem_zero_struct(dst) mem_zero((dst), sizeof(*(dst)))

#define swap_t(T, a, b) do { T t__ = a; a = b; b = t__; } while (0)
#define sign_t(T, x) ((T)((x) > 0) - (T)((x) < 0))
#define abs_t(T, x) (sign_t(T, x)*(x))
#define round_t(T, x) (T)((x) + 0.5f)

#define radians_f32(x) ((x)*pi32/180.0f)

#define internal static
#define global static
#define local static

#define false 0
#define true 1

////////////////////
// NOTE: Basic Types

typedef size_t   uxx;
typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t   s8;
typedef int16_t  s16;
typedef int32_t  s32;
typedef int64_t  s64;
typedef s8       b8;
typedef s16      b16;
typedef s32      b32;
typedef s64      b64;
typedef float    f32;
typedef double   f64;

////////////////////////
// NOTE: Basic Constants

global uxx uxx_max = SIZE_MAX;

global u8  u8_max  = 0xff;
global u16 u16_max = 0xffff;
global u32 u32_max = 0xffffffff;
global u64 u64_max = 0xffffffffffffffffull;

global s8  s8_max  =  (s8)0x7f;
global s16 s16_max = (s16)0x7fff;
global s32 s32_max = (s32)0x7fffffff;
global s64 s64_max = (s64)0x7fffffffffffffffll;

global s8  s8_min  =  (s8)0x80;
global s16 s16_min = (s16)0x8000;
global s32 s32_min = (s32)0x80000000;
global s64 s64_min = (s64)0x8000000000000000ll;

global f32 f32_max = FLT_MAX;
global f32 f32_min = -FLT_MAX;

global f32 pi32 = 3.141592653589793f;
global f64 pi64 = 3.141592653589793;

global f32 tau32 = 6.283185307179586f;
global f64 tau64 = 6.283185307179586;

#endif // KRUEGER_BASE_CORE_H
