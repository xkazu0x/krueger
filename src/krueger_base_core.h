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

//////////////////////////
// NOTE: Codebase Keywords

#define internal static
#define global static
#define local static

#define false 0
#define true 1

////////////////
// NOTE: Asserts

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

//////////////
// NOTE: Units

#define KB(n) (((u64)(n))<<10)
#define MB(n) (((u64)(n))<<20)
#define GB(n) (((u64)(n))<<30)
#define TB(n) (((u64)(n))<<40)
#define thousand(n) ((n)*1000)
#define million(n)  ((n)*1000000)
#define billion(n)  ((n)*1000000000)

////////////////////////////
// NOTE: Clamps, Mins, Maxes

#define min(a, b) ((a)<(b)?(a):(b))
#define max(a, b) ((a)>(b)?(a):(b))
#define clamp_top(a, x) min(a, x);
#define clamp_bot(x, b) max(x, b);
#define clamp(a, x, b) (((x)<(a))?(a):((x)>(b))?(b):(x))

////////////////////////////////
// NOTE: Memory Operation Macros

#define mem_cpy(dst, src, size)     memmove((dst), (src), (size))
#define mem_set(dst, byte, size)    memset((dst), (byte), (size))
#define mem_cmp(a, b, size)         memcmp((a), (b), (size))

#define mem_zero(dst, size)         mem_set((dst), 0, (size))
#define mem_zero_struct(dst)        mem_zero((dst), sizeof(*(dst)))
#define mem_zero_array(dst)         mem_zero((dst), sizeof(dst));
#define mem_zero_typed(dst, count)  mem_zero((dst), sizeof(*(dst))*(count))

///////////////////////////
// NOTE: Linked List Macros

// NOTE: Doubly Linked Lists
#define dll_push_back_np(f, l, n, next, prev) \
  (((f)==0) ? \
    ((f)=(l)=(n),(n)->next=(n)->prev=0):\
    ((n)->prev=(l),(l)->next=(n),(l)=(n),(n)->next=0))
#define dll_remove_np(f, l, n, next, prev) \
  (((n)==(f) ? (f)=(n)->next : (0)), \
   ((n)==(l) ? (l)=(l)->prev : (0)), \
   ((n)->prev==0 ? (0) : ((n)->prev->next=(n)->next)), \
   ((n)->next==0 ? (0) : ((n)->next->prev=(n)->prev)))
#define dll_push_back(f, l, n) dll_push_back_np(f, l, n, next, prev)
#define dll_push_front(f, l, n) dll_push_back_np(l, f, n, prev, next)
#define dll_remove(f, l, n) dll_remove_np(f, l, n, next, prev)

// NOTE: Singly Linked Lists (Queues)
#define sll_queue_push_n(f, l, n, next) \
  (((f)==0) ? \
    ((f)=(l)=(n),(n)->next=0) : \
    ((l)->next=(n),(l)=(n),(n)->next=0))
#define sll_queue_pop_n(f, l, next) \
  ((f)==(l) ? ((f)=(l)=0) : ((f)=(f)->next))
#define sll_queue_push(f, l, n) sll_queue_push_n(f, l, n, next)
#define sll_queue_pop(f, l) sll_queue_pop_n(f, l, next)

// NOTE: Singly Linked List (Stacks)
#define sll_stack_push_n(f, n, next) ((n)->next=(f), (f)=(n))
#define sll_stack_pop_n(f, next) ((f)=(f)->next)
#define sll_stack_push(f, n) sll_stack_push_n(f, n, next);
#define sll_stack_pop(f) sll_stack_pop_n(f, next);

////////////////////////////
// NOTE: Misc. Helper Macros

#define array_count(x) (sizeof(x)/sizeof(*(x)))
#define square(x) ((x)*(x))
#define cast(T) (T)

#define swap_t(T, a, b) do { T t__ = a; a = b; b = t__; } while (0)
#define sign_t(T, x) (cast(T) ((x) > 0) - cast(T) ((x) < 0))
#define abs_t(T, x) (sign_t(T, x)*(x))
#define round_t(T, x) cast(T) ((x) + 0.5f)

#define radians_pi32(x) ((x)*pi32/180.0f)
#define radians_tau32(x) ((x)*tau32/360.0f)

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

global s8  s8_max  = cast(s8)  0x7f;
global s16 s16_max = cast(s16) 0x7fff;
global s32 s32_max = cast(s32) 0x7fffffff;
global s64 s64_max = cast(s64) 0x7fffffffffffffffll;

global s8  s8_min  = cast(s8)  0x80;
global s16 s16_min = cast(s16) 0x8000;
global s32 s32_min = cast(s32) 0x80000000;
global s64 s64_min = cast(s64) 0x8000000000000000ll;

global f32 f32_max = FLT_MAX;
global f32 f32_min = -FLT_MAX;

global f32 pi32 = 3.141592653589793f;
global f64 pi64 = 3.141592653589793;

global f32 tau32 = 6.283185307179586f;
global f64 tau64 = 6.283185307179586;

global const u32 bitmask1  = 0x00000001;
global const u32 bitmask2  = 0x00000003;
global const u32 bitmask3  = 0x00000007;
global const u32 bitmask4  = 0x0000000f;
global const u32 bitmask5  = 0x0000001f;
global const u32 bitmask6  = 0x0000003f;
global const u32 bitmask7  = 0x0000007f;
global const u32 bitmask8  = 0x000000ff;
global const u32 bitmask9  = 0x000001ff;
global const u32 bitmask10 = 0x000003ff;
global const u32 bitmask11 = 0x000007ff;
global const u32 bitmask12 = 0x00000fff;
global const u32 bitmask13 = 0x00001fff;
global const u32 bitmask14 = 0x00003fff;
global const u32 bitmask15 = 0x00007fff;
global const u32 bitmask16 = 0x0000ffff;
global const u32 bitmask17 = 0x0001ffff;
global const u32 bitmask18 = 0x0003ffff;
global const u32 bitmask19 = 0x0007ffff;
global const u32 bitmask20 = 0x000fffff;
global const u32 bitmask21 = 0x001fffff;
global const u32 bitmask22 = 0x003fffff;
global const u32 bitmask23 = 0x007fffff;
global const u32 bitmask24 = 0x00ffffff;
global const u32 bitmask25 = 0x01ffffff;
global const u32 bitmask26 = 0x03ffffff;
global const u32 bitmask27 = 0x07ffffff;
global const u32 bitmask28 = 0x0fffffff;
global const u32 bitmask29 = 0x1fffffff;
global const u32 bitmask30 = 0x3fffffff;
global const u32 bitmask31 = 0x7fffffff;
global const u32 bitmask32 = 0xffffffff;

global const u32 bit1  = (1<<0);
global const u32 bit2  = (1<<1);
global const u32 bit3  = (1<<2);
global const u32 bit4  = (1<<3);
global const u32 bit5  = (1<<4);
global const u32 bit6  = (1<<5);
global const u32 bit7  = (1<<6);
global const u32 bit8  = (1<<7);
global const u32 bit9  = (1<<8);
global const u32 bit10 = (1<<9);
global const u32 bit11 = (1<<10);
global const u32 bit12 = (1<<11);
global const u32 bit13 = (1<<12);
global const u32 bit14 = (1<<13);
global const u32 bit15 = (1<<14);
global const u32 bit16 = (1<<15);
global const u32 bit17 = (1<<16);
global const u32 bit18 = (1<<17);
global const u32 bit19 = (1<<18);
global const u32 bit20 = (1<<19);
global const u32 bit21 = (1<<20);
global const u32 bit22 = (1<<21);
global const u32 bit23 = (1<<22);
global const u32 bit24 = (1<<23);
global const u32 bit25 = (1<<24);
global const u32 bit26 = (1<<25);
global const u32 bit27 = (1<<26);
global const u32 bit28 = (1<<27);
global const u32 bit29 = (1<<28);
global const u32 bit30 = (1<<29);
global const u32 bit31 = (1<<30);
global const u32 bit32 = cast(u32) (1<<31);

/////////////
// NOTE: Time

typedef struct {
  u16 year;
  u16 month;
  u16 day;
  u16 hour;
  u16 min;
  u16 sec;
  u16 msec;
} Date_Time;

typedef u64 Dense_Time;

internal Dense_Time dense_time_from_date_time(Date_Time time);
internal Date_Time date_time_from_dense_time(Dense_Time time);

#endif // KRUEGER_BASE_CORE_H
