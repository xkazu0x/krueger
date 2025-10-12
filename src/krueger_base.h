#ifndef KRUEGER_BASE_H
#define KRUEGER_BASE_H

//////////////////////////////////
// NOTE: Standard Library Includes

#include <stdlib.h> // TODO: remove this
#include <stdint.h>
#include <stddef.h>
#include <float.h>
#include <math.h>

///////////////////////////////
// NOTE: Clang Context Cracking

#if defined(__clang__)
#define COMPILER_CLANG 1

#if defined(_WIN32)
#define PLATFORM_WINDOWS 1
#elif defined(__gnu_linux__) || defined(__linux__)
#define PLATFORM_LINUX 1
#elif defined(__APPLE__) && defined(__MACH__)
#define PLATFORM_MAC 1
#error compiler/platform is not supported
#endif

#if defined(__amd64__) || defined(__amd64) || defined(__x86_64__) || defined(__x86_64)
#define ARCH_X64 1
#elif defined(i386) || defined(__i386) || defined(__i386__)
#define ARCH_X86 1
#elif defined(__aarch64__)
#define ARCH_ARM64 1
#elif defined(__arm__)
#define ARCH_ARM32 1
#else
#error architecture is not supported
#endif

//////////////////////////////
// NOTE: MSVC Context Cracking

#elif defined(_MSC_VER)
#define COMPILER_MSVC 1

#if defined(_WIN32)
#define PLATFORM_WINDOWS 1
#else
#error compiler/platform is not supported
#endif

#if defined(_M_AMD64)
#define ARCH_X64 1
#elif defined(_M_IX86)
#define ARCH_X86 1
#elif defined(_M_ARM64)
#define ARCH_ARM64 1
#elif defined(_M_ARM)
#define ARCH_ARM32 1
#else
#error architecture is not supported
#endif

/////////////////////////////
// NOTE: GCC Context Cracking

#elif defined(__GNUC__) || defined(__GNUG__)
#define COMPILER_GCC 1

#if defined(_WIN32)
#define PLATFORM_WINDOWS 1
#elif defined(__gnu_linux__) || defined(__linux__)
#define PLATFORM_LINUX 1
#elif defined(__APPLE__) && defined(__MACH__)
#define PLATFORM_MAC 1
#else
#error compiler/platform is not supported
#endif

#if defined(__amd64__) || defined(__amd64) || defined(__x86_64__) || defined(__x86_64)
#define ARCH_X64 1
#elif defined(i386) || defined(__i386) || defined(__i386__)
#define ARCH_X86 1
#elif defined(__aarch64__)
#define ARCH_ARM64 1
#elif defined(__arm__)
#define ARCH_ARM32 1
#else
#error architecture is not supported
#endif

#else
#error compiler is not supported
#endif

//////////////////////////////
// NOTE: Build Option Cracking

#if !defined(BUILD_DEBUG)
#define BUILD_DEBUG 1
#endif

///////////////////////////////////
// NOTE: Zero All Undefined Options

#if !defined(COMPILER_CLANG)
#define COMPILER_CLANG 0
#endif
#if !defined(COMPILER_MSVC)
#define COMPILER_MSVC 0
#endif
#if !defined(COMPILER_GCC)
#define COMPILER_GCC 0
#endif
#if !defined(PLATFORM_WINDOWS)
#define PLATFORM_WINDOWS 0
#endif
#if !defined(PLATFORM_LINUX)
#define PLATFORM_LINUX 0
#endif
#if !defined(PLATFORM_MAC)
#define PLATFORM_MAC 0
#endif
#if !defined(ARCH_X64)
#define ARCH_X64 0
#endif
#if !defined(ARCH_X86)
#define ARCH_X86 0
#endif
#if !defined(ARCH_ARM64)
#define ARCH_ARM64 0
#endif
#if !defined(ARCH_ARM32)
#define ARCH_ARM32 0
#endif

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

#define internal static
#define global static
#define local static

#define false 0
#define true 1

#define KB(n) (((u64)(n))<<10)
#define MB(n) (((u64)(n))<<20)
#define GB(n) (((u64)(n))<<30)
#define TB(n) (((u64)(n))<<40)
#define thousand(n) ((n)*1000)
#define million(n)  ((n)*1000000)
#define billion(n)  ((n)*1000000000)

#define MIN(a, b) ((a)<(b)?(a):(b))
#define MAX(a, b) ((a)>(b)?(a):(b))
#define clamp_top(a, x) MIN(a, x);
#define clamp_bot(x, b) MAX(x, b);

#define array_count(x) (sizeof(x)/sizeof(*(x)))
#define square(x) ((x)*(x))

#define swap_t(T, a, b) do { T t__ = a; a = b; b = t__; } while (0)
#define sign_t(T, x) ((T)((x) > 0) - (T)((x) < 0))
#define abs_t(T, x) (sign_t(T, x)*(x))

#define radians_f32(x) ((x)*pi32/180.0f)

///////////////////
// NOTE: Base Types

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

/////////////
// NOTE: Math

#define sqrt_f32(x) sqrtf(x)
#define pow_f32(a, b) powf((a), (b))
#define floor_f32(x) floorf(x)
#define ceil_f32(x) ceilf(x)
#define sin_f32(x) sinf(x)
#define cos_f32(x) cosf(x)
#define tan_f32(x) tanf(x)

internal f32 lerp_f32(f32 a, f32 b, f32 t);

////////////////
// NOTE: Vector2

typedef struct {
  f32 x, y;
} Vector2;

internal Vector2 make_vector2(f32 x, f32 y);
internal Vector2 vector2_add(Vector2 a, Vector2 b);
internal Vector2 vector2_sub(Vector2 a, Vector2 b);
internal Vector2 vector2_mul(Vector2 v, f32 s);
internal Vector2 vector2_div(Vector2 v, f32 s);
internal Vector2 vector2_hadamard(Vector2 a, Vector2 b);
internal f32 vector2_dot(Vector2 a, Vector2 b);
internal f32 vector2_length_square(Vector2 v);
internal f32 vector2_length(Vector2 v);
internal Vector2 vector2_normalize(Vector2 v);
internal Vector2 vector2_lerp(Vector2 a, Vector2 b, f32 t);

////////////////
// NOTE: Vector3

typedef union {
  struct {
    f32 x, y, z;
  };
  struct {
    f32 r, g, b;
  };
  struct {
    Vector2 xy;
    f32 _z0;
  };
  struct {
    Vector2 rg;
    f32 _b0;
  };
} Vector3;

internal Vector3 make_vector3(f32 x, f32 y, f32 z);
internal Vector3 vector3_add(Vector3 a, Vector3 b);
internal Vector3 vector3_sub(Vector3 a, Vector3 b);
internal Vector3 vector3_mul(Vector3 v, f32 s);
internal Vector3 vector3_div(Vector3 v, f32 s);
internal Vector3 vector3_hadamard(Vector3 a, Vector3 b);
internal Vector3 vector3_cross(Vector3 a, Vector3 b);
internal f32 vector3_dot(Vector3 a, Vector3 b);
internal f32 vector3_length_square(Vector3 v);
internal f32 vector3_length(Vector3 v);
internal Vector3 vector3_normalize(Vector3 v);
internal Vector3 vector3_lerp(Vector3 a, Vector3 b, f32 t);

////////////////
// NOTE: Vector4

typedef union {
  struct {
    f32 x, y, z, w;
  };
  struct {
    f32 r, g, b, a;
  };
  struct {
    Vector3 xyz;
    f32 _w0;
  };
  struct {
    Vector3 rgb;
    f32 _a0;
  };
  struct {
    Vector2 xy;
    Vector2 zw;
  };
  struct {
    Vector2 rg;
    Vector2 ba;
  };
} Vector4;

internal Vector4 make_vector4(f32 x, f32 y, f32 z, f32 w);
internal Vector4 vector4_from_vector3(Vector3 v, f32 w);
internal Vector4 vector4_add(Vector4 a, Vector4 b);
internal Vector4 vector4_sub(Vector4 a, Vector4 b);
internal Vector4 vector4_mul(Vector4 v, f32 s);
internal Vector4 vector4_div(Vector4 v, f32 s);
internal Vector4 vector4_hadamard(Vector4 a, Vector4 b);
internal f32 vector4_dot(Vector4 a, Vector4 b);
internal f32 vector4_length_square(Vector4 v);
internal f32 vector4_length(Vector4 v);
internal Vector4 vector4_normalize(Vector4 v);
internal Vector4 vector4_lerp(Vector4 a, Vector4 b, f32 t);

//////////////////
// NOTE: Matrix4x4

typedef struct {
  f32 m[4][4]; // NOTE: Column Major
} Matrix4x4;

internal Matrix4x4 make_matrix4x4(f32 d);
internal Matrix4x4 matrix4x4_perspective(f32 fov_deg, f32 aspect_ratio, f32 z_near, f32 z_far);
internal Matrix4x4 matrix4x4_scale(Vector3 scale);
internal Matrix4x4 matrix4x4_translate(f32 x, f32 y, f32 z);
internal Matrix4x4 matrix4x4_rotate(Vector3 axis, f32 t);
internal Matrix4x4 matrix4x4_mul(Matrix4x4 a, Matrix4x4 b);
internal Vector4 matrix4x4_mul_vector4(Matrix4x4 m, Vector4 v);

//////////////
// NOTE: Arena
typedef struct {
  uxx reserve_size;
  uxx commit_size;
  u8 *memory;
} Arena;

#define push_array(a, T, c) (T *)arena_push((a), sizeof(T)*(c))

internal Arena arena_alloc(uxx reserve_size);
internal void arena_free(Arena *arena);
internal void *arena_push(Arena *arena, uxx commit_size);

////////////////////////
// NOTE: Stretchy Buffer

typedef struct {
  uxx len;
  uxx cap;
  u8 *ptr[0];
} Buffer_Header;

#define buf__hdr(b) ((Buffer_Header *)((u8 *)(b) - offsetof(Buffer_Header, ptr)))
#define buf__fits(b, n) (buf_len(b) + (n) <= buf_cap(b))
#define buf__fit(b, n) (buf__fits((b), (n)) ? 0 : ((b) = buf__grow((b), buf_len(b) + (n), sizeof(*(b)))))

#define buf_len(b) ((b) ? buf__hdr(b)->len : 0)
#define buf_cap(b) ((b) ? buf__hdr(b)->cap : 0)
#define buf_push(b, x) (buf__fit((b), 1), (b)[buf__hdr(b)->len++] = (x))
#define buf_free(b) ((b) ? (free(buf__hdr(b)), (b) = 0) : 0)
#define buf_clear(b) ((b) ? buf__hdr(b)->len = 0 : 0)

internal void *buf__grow(const void *buf, uxx new_len, uxx elem_size);

///////////////
// NOTE: String

internal uxx cstr_len(char *cstr);
internal b32 cstr_match(char *a, char *b);
internal uxx cstr_encode(char *cstr);

#endif // KRUEGER_BASE_H
