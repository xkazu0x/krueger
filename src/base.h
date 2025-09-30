#ifndef BASE_H
#define BASE_H

#if !defined(BUILD_DEBUG)
# define BUILD_DEBUG 1
#endif

#if BUILD_DEBUG
#include <assert.h>
#else
#define assert(x) (void)(x)
#endif

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

typedef struct {
    f32 x, y;
} Vector2;

internal Vector2
make_vector2(f32 x, f32 y) {
    Vector2 result = { 
        .x = x,
        .y = y,
    };
    return(result);
}

internal Vector2
vector2_add(Vector2 a, Vector2 b) {
    Vector2 result = {
        .x = a.x + b.x,
        .y = a.y + b.y,
    };
    return(result);
}

internal Vector2
vector2_sub(Vector2 a, Vector2 b) {
    Vector2 result = {
        .x = a.x - b.x,
        .y = a.y - b.y,
    };
    return(result);
}

internal Vector2
vector2_mul(Vector2 v, f32 s) {
    Vector2 result = {
        .x = v.x*s,
        .y = v.y*s,
    };
    return(result);
}

internal Vector2
vector2_div(Vector2 v, f32 s) {
    Vector2 result = {
        .x = v.x/s,
        .y = v.y/s,
    };
    return(result);
}

internal Vector2
vector2_hadamard(Vector2 a, Vector2 b) {
    Vector2 result = {
        .x = a.x*b.x,
        .y = a.y*b.y,
    };
    return(result);
}

internal f32
vector2_dot(Vector2 a, Vector2 b) {
    f32 result = a.x*b.x + a.y*b.y;
    return(result);
}

internal f32
vector2_length_square(Vector2 v) {
    f32 result = vector2_dot(v, v);
    return(result);
}

internal f32 
vector2_length(Vector2 v) {
    f32 result = sqrt_f32(vector2_length_square(v));
    return(result);
}

internal Vector2 
vector2_normalize(Vector2 v) {
    Vector2 result = vector2_mul(v, (1.0f / vector2_length(v)));
    return(result);
}

internal Vector2
vector2_lerp(Vector2 a, Vector2 b, f32 t) {
    Vector2 result = {
        .x = lerp_f32(a.x, b.x, t),
        .y = lerp_f32(a.y, b.y, t),
    };
    return(result);
}

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
vector3_mul(Vector3 v, f32 s) {
    Vector3 result = {
        .x = v.x*s,
        .y = v.y*s,
        .z = v.z*s,
    };
    return(result);
}

internal Vector3
vector3_div(Vector3 v, f32 s) {
    Vector3 result = {
        .x = v.x/s,
        .y = v.y/s,
        .z = v.z/s,
    };
    return(result);
}

internal Vector3
vector3_hadamard(Vector3 a, Vector3 b) {
    Vector3 result = {
        .x = a.x*b.x,
        .y = a.y*b.y,
        .z = a.z*b.z,
    };
    return(result);
}

internal f32
vector3_dot(Vector3 a, Vector3 b) {
    f32 result = a.x*b.x + a.y*b.y + a.z*b.z;
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
    Vector3 result = vector3_mul(v, (1.0f / vector3_length(v)));
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

internal Vector4
make_vector4(f32 x, f32 y, f32 z, f32 w) {
    Vector4 result = { 
        .x = x,
        .y = y,
        .z = z,
        .w = w,
    };
    return(result);
}

internal Vector4
vector4_from_vector3(Vector3 v, f32 w) {
    Vector4 result = {
        .x = v.x,
        .y = v.y,
        .z = v.z,
        .w = w,
    };
    return(result);
}

internal Vector4
vector4_add(Vector4 a, Vector4 b) {
    Vector4 result = {
        .x = a.x + b.x,
        .y = a.y + b.y,
        .z = a.z + b.z,
        .w = a.w + b.w,
    };
    return(result);
}

internal Vector4
vector4_sub(Vector4 a, Vector4 b) {
    Vector4 result = {
        .x = a.x - b.x,
        .y = a.y - b.y,
        .z = a.z - b.z,
        .w = a.w - b.w,
    };
    return(result);
}

internal Vector4
vector4_mul(Vector4 v, f32 s) {
    Vector4 result = {
        .x = v.x*s,
        .y = v.y*s,
        .z = v.z*s,
        .w = v.w*s,
    };
    return(result);
}

internal Vector4
vector4_div(Vector4 v, f32 s) {
    Vector4 result = {
        .x = v.x/s,
        .y = v.y/s,
        .z = v.z/s,
        .w = v.w/s,
    };
    return(result);
}

internal Vector4
vector4_hadamard(Vector4 a, Vector4 b) {
    Vector4 result = {
        .x = a.x*b.x,
        .y = a.y*b.y,
        .z = a.z*b.z,
        .w = a.w*b.w,
    };
    return(result);
}

internal f32
vector4_dot(Vector4 a, Vector4 b) {
    f32 result = a.x*b.x + a.y*b.y + a.z*b.z + a.w*b.w;
    return(result);
}

internal f32
vector4_length_square(Vector4 v) {
    f32 result = vector4_dot(v, v);
    return(result);
}

internal f32 
vector4_length(Vector4 v) {
    f32 result = sqrt_f32(vector4_length_square(v));
    return(result);
}

internal Vector4 
vector4_normalize(Vector4 v) {
    Vector4 result = vector4_mul(v, (1.0f / vector4_length(v)));
    return(result);
}

internal Vector4
vector4_lerp(Vector4 a, Vector4 b, f32 t) {
    Vector4 result = {
        .x = lerp_f32(a.x, b.x, t),
        .y = lerp_f32(a.y, b.y, t),
        .z = lerp_f32(a.z, b.z, t),
        .w = lerp_f32(a.w, b.w, t),
    };
    return(result);
}

// NOTE: Column Major
typedef struct {
    f32 m[4][4];
} Matrix4x4;

internal Matrix4x4
make_matrix4x4(f32 d) {
    Matrix4x4 result = {0};
    result.m[0][0] = d;
    result.m[1][1] = d;
    result.m[2][2] = d;
    result.m[3][3] = d;
    return(result);
}

internal Matrix4x4
matrix4x4_perspective(f32 fov_deg, f32 aspect_ratio, f32 z_near, f32 z_far) {
    // NOTE: Row Major
    Matrix4x4 result = make_matrix4x4(1.0f);
    f32 fov_rad = 1.0f / tan_f32(radians_f32(fov_deg*0.5f));
    result.m[0][0] = fov_rad*aspect_ratio;
    result.m[1][1] = fov_rad;
    result.m[2][2] = z_far / (z_far - z_near);
    result.m[2][3] = 1.0f;
    result.m[3][2] = (z_far*z_near) / (z_far - z_near);
    result.m[3][3] = 0.0f;
    return(result);
}

internal Matrix4x4
matrix4x4_scale(Vector3 scale) {
    Matrix4x4 result = make_matrix4x4(1.0f);
    result.m[0][0] = scale.x;
    result.m[1][1] = scale.y;
    result.m[2][2] = scale.z;
    return(result);
}

internal Matrix4x4
matrix4x4_translate(f32 x, f32 y, f32 z) {
    Matrix4x4 result = make_matrix4x4(1.0f);
    result.m[3][0] = x;
    result.m[3][1] = y;
    result.m[3][2] = z;
    return(result);
}

internal Matrix4x4
matrix4x4_rotate(Vector3 axis, f32 t) {
    Matrix4x4 result = make_matrix4x4(1.0f);
    axis = vector3_normalize(axis);
    f32 sin_theta = sin_f32(t);
    f32 cos_theta = cos_f32(t);
    f32 cos_value = 1.0f - cos_theta;
    result.m[0][0] = (axis.x*axis.x*cos_value) + cos_theta;
    result.m[0][1] = (axis.x*axis.y*cos_value) + (axis.z*sin_theta);
    result.m[0][2] = (axis.x*axis.z*cos_value) - (axis.y*sin_theta);
    result.m[1][0] = (axis.y*axis.x*cos_value) - (axis.z*sin_theta);
    result.m[1][1] = (axis.y*axis.y*cos_value) + cos_theta;
    result.m[1][2] = (axis.y*axis.z*cos_value) + (axis.x*sin_theta);
    result.m[2][0] = (axis.z*axis.x*cos_value) + (axis.y*sin_theta);
    result.m[2][1] = (axis.z*axis.y*cos_value) - (axis.x*sin_theta);
    result.m[2][2] = (axis.z*axis.z*cos_value) + cos_theta;
    return(result);
}

internal Matrix4x4
matrix4x4_mul(Matrix4x4 a, Matrix4x4 b) {
    Matrix4x4 result = {0};
    for (u32 j = 0; j < 4; ++j) {
        for (u32 i = 0; i < 4; ++i) {
            result.m[i][j] = (a.m[0][j]*b.m[i][0] +
                              a.m[1][j]*b.m[i][1] +
                              a.m[2][j]*b.m[i][2] +
                              a.m[3][j]*b.m[i][3]);
        }
    }
    return(result);
}

internal Vector4
matrix4x4_mul_vector4(Matrix4x4 m, Vector4 v) {
    Vector4 result = {
        .x = v.x*m.m[0][0] + v.y*m.m[1][0] + v.z*m.m[2][0] + m.m[3][0],
        .y = v.x*m.m[0][1] + v.y*m.m[1][1] + v.z*m.m[2][1] + m.m[3][1],
        .z = v.x*m.m[0][2] + v.y*m.m[1][2] + v.z*m.m[2][2] + m.m[3][2],
        .w = v.x*m.m[0][3] + v.y*m.m[1][3] + v.z*m.m[2][3] + m.m[3][3],
    };
    return(result);
}

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

// TODO: change malloc to os specific malloc
#include <stdlib.h>
internal void *
buf__grow(const void *buf, uxx new_len, uxx elem_size) {
    uxx new_cap = max(1 + 2*buf_cap(buf), new_len);
    assert(new_len <= new_cap);
    uxx new_size = offsetof(Buffer_Header, ptr) + new_cap*elem_size;
    Buffer_Header *header;
    if (buf) {
        header = realloc(buf__hdr(buf), new_size);
    } else {
        header = malloc(new_size);
        header->len = 0;
    }
    header->cap = new_cap;
    return(header->ptr);
}

#endif // BASE_H
