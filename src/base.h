#ifndef BASE_H
#define BASE_H

#define internal static
#define global static
#define local static

#define false 0
#define true 1

#define square(x) ((x)*(x))
#define array_count(x) (sizeof(x)/sizeof(*(x)))

#define min(a, b) ((a)<(b)?(a):(b))
#define max(a, b) ((a)>(b)?(a):(b))
#define clamp_top(a, x) min(a, x);
#define clamp_bot(x, b) max(x, b);

#define swap_t(T, a, b) do { T t__ = a; a = b; b = t__; } while (0)
#define sign_t(T, x) ((T)((x) > 0) - (T)((x) < 0))
#define abs_t(T, x) (sign_t(T, x)*(x))

#include <stdint.h>
#include <stddef.h>
typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef size_t   uxx;
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

#include <float.h>
global f32 f32_max = FLT_MAX;
global f32 f32_min = -FLT_MAX;

global f32 pi32 = 3.141592653589793f;
global f64 pi64 = 3.141592653589793;

global f32 tau32 = 6.283185307179586f;
global f64 tau64 = 6.283185307179586;

#include <math.h>
#define sqrt_f32(x) sqrtf(x)
#define pow_f32(a, b) powf((a), (b))
#define sin_f32(x) sinf(x)
#define cos_f32(x) cosf(x)
#define tan_f32(x) tanf(x)

#define radians_f32(x) ((x)*pi32/180.0f)

internal f32
lerp_f32(f32 a, f32 b, f32 t) {
    f32 result = a + t*(b - a);
    return(result);
}

typedef union {
    struct {
        f32 x, y, z;
    };
    struct {
        f32 r, g, b;
    };
} Vector3;

internal Vector3
make_vector3(f32 x, f32 y, f32 z) {
    Vector3 result = { 
        .x = x,
        .y = y,
        .z = z,
    };
    return(result);
}

internal Vector3
vector3_add(Vector3 a, Vector3 b) {
    Vector3 result = {
        .x = a.x + b.x,
        .y = a.y + b.y,
        .z = a.z + b.z,
    };
    return(result);
}

internal Vector3
vector3_sub(Vector3 a, Vector3 b) {
    Vector3 result = {
        .x = a.x - b.x,
        .y = a.y - b.y,
        .z = a.z - b.z,
    };
    return(result);
}

internal Vector3
vector3_mul(Vector3 a, Vector3 b) {
    Vector3 result = {
        .x = a.x*b.x,
        .y = a.y*b.y,
        .z = a.z*b.z,
    };
    return(result);
}

internal Vector3
vector3_scale(Vector3 v, f32 s) {
    Vector3 result = {
        .x = v.x*s,
        .y = v.y*s,
        .z = v.z*s,
    };
    return(result);
}

internal f32
vector3_dot(Vector3 a, Vector3 b) {
    f32 result = a.x*b.x + a.y*b.y + a.z*b.z;
    return(result);
}

internal f32
vector3_length_square(Vector3 v) {
    f32 result = vector3_dot(v, v);
    return(result);
}

internal f32 
vector3_length(Vector3 v) {
    f32 result = sqrt_f32(vector3_length_square(v));
    return(result);
}

internal Vector3 
vector3_normalize(Vector3 v) {
    Vector3 result = vector3_scale(v, (1.0f / vector3_length(v)));
    return(result);
}

internal Vector3
vector3_cross(Vector3 a, Vector3 b) {
    Vector3 result = {
        .x = a.y*b.z - a.z*b.y,
        .y = a.z*b.x - a.x*b.z,
        .z = a.x*b.y - a.y*b.x,
    };
    return(result);
}

internal Vector3
vector3_lerp(Vector3 a, Vector3 b, f32 t) {
    Vector3 result = {
        .x = lerp_f32(a.x, b.x, t),
        .y = lerp_f32(a.y, b.y, t),
        .z = lerp_f32(a.z, b.z, t),
    };
    return(result);
}

#endif // BASE_H
