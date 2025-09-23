#ifndef BASE_H
#define BASE_H

#define internal static
#define global static

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

typedef union {
    struct {
        f32 x, y, z;
    };
    struct {
        f32 r, g, b;
    };
} vec3;

internal vec3
make_vec3(f32 x, f32 y, f32 z) {
    vec3 result = { 
        .x = x,
        .y = y,
        .z = z,
    };
    return(result);
}

internal vec3
vec3_add(vec3 a, vec3 b) {
    vec3 result = {
        .x = a.x + b.x,
        .y = a.y + b.y,
        .z = a.z + b.z,
    };
    return(result);
}

internal vec3
vec3_sub(vec3 a, vec3 b) {
    vec3 result = {
        .x = a.x - b.x,
        .y = a.y - b.y,
        .z = a.z - b.z,
    };
    return(result);
}

internal vec3
vec3_scale(vec3 v, f32 s) {
    vec3 result = {
        .x = v.x*s,
        .y = v.y*s,
        .z = v.z*s,
    };
    return(result);
}

internal f32
vec3_dot(vec3 a, vec3 b) {
    f32 result = a.x*b.x + a.y*b.y + a.z*b.z;
    return(result);
}

internal f32
vec3_length_square(vec3 v) {
    f32 result = vec3_dot(v, v);
    return(result);
}

internal f32 
vec3_length(vec3 v) {
    f32 result = sqrt_f32(vec3_length_square(v));
    return(result);
}

#define square(x) ((x)*(x))

internal vec3 
vec3_normalize(vec3 v) {
#if 0
    vec3 result = vec3_scale(v, (1.0f / vec3_length(v)));
#else
    vec3 result = {0};
    f32 length_square = vec3_length_square(v);
    if (length_square > square(0.0001f)) {
        result = vec3_scale(v, (1.0f / sqrt_f32(length_square)));
    }
#endif
    return(result);
}

internal vec3
vec3_cross(vec3 a, vec3 b) {
    vec3 result = {
        .x = a.y*b.z - a.z*b.y,
        .y = a.z*b.x - a.x*b.z,
        .z = a.x*b.y - a.y*b.x,
    };
    return(result);
}

#endif // BASE_H
