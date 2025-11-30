#ifndef KRUEGER_BASE_MATH_H
#define KRUEGER_BASE_MATH_H

#define sqrt_f32(x) sqrtf(x)
#define pow_f32(a, b) powf((a), (b))
#define floor_f32(x) floorf(x)
#define ceil_f32(x) ceilf(x)
#define sin_f32(x) sinf(x)
#define cos_f32(x) cosf(x)
#define tan_f32(x) tanf(x)

#define lerp_t(T, a, b, t) (T)((a) + (t)*((b) - (a)))
internal f32 lerp_f32(f32 a, f32 b, f32 t);

////////////////
// NOTE: Vector2

typedef union {
  struct { f32 x, y; };
  struct { f32 u, v; };
  f32 buf[2];
} Vector2;

#define vec2(x, y) ((Vector2){{(x), (y)}})
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
  struct { f32 x, y, z; };
  struct { f32 r, g, b; };
  struct { Vector2 xy; f32 _z0; };
  struct { Vector2 rg; f32 _b0; };
  f32 buf[3];
} Vector3;

#define vec3(x, y, z) ((Vector3){{(x), (y), (z)}})
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
  struct { f32 x, y, z, w; };
  struct { f32 r, g, b, a; };
  struct { Vector2 xy; Vector2 zw; };
  struct { Vector2 rg; Vector2 ba; };
  struct { Vector3 xyz; f32 _w0; };
  struct { Vector3 rgb; f32 _a0; };
  f32 buf[4];
} Vector4;

#define vec4(x, y, z, w) ((Vector4){{(x), (y), (z), (w)}})
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
  f32 buf[4][4]; // NOTE: Column Major
} Matrix4x4;

internal Matrix4x4 make_matrix4x4(f32 d);
internal Matrix4x4 matrix4x4_perspective(f32 fov_deg, f32 aspect_ratio, f32 z_near, f32 z_far);
internal Matrix4x4 matrix4x4_scale(Vector3 scale);
internal Matrix4x4 matrix4x4_translate(f32 x, f32 y, f32 z);
internal Matrix4x4 matrix4x4_rotate(Vector3 axis, f32 t);
internal Matrix4x4 matrix4x4_mul(Matrix4x4 a, Matrix4x4 b);
internal Vector4 matrix4x4_mul_vector4(Matrix4x4 m, Vector4 v);

#endif // KRUEGER_BASE_MATH_H
