#ifndef KRUEGER_BASE_MATH_C
#define KRUEGER_BASE_MATH_C

internal f32
lerp_f32(f32 a, f32 b, f32 t) {
  f32 result = a + t*(b - a);
  return(result);
}

////////////////
// NOTE: Vector2

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

////////////////
// NOTE: Vector3

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

////////////////
// NOTE: Vector4

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

//////////////////
// NOTE: Matrix4x4

internal Matrix4x4
make_matrix4x4(f32 d) {
  Matrix4x4 result = {0};
  result.buf[0][0] = d;
  result.buf[1][1] = d;
  result.buf[2][2] = d;
  result.buf[3][3] = d;
  return(result);
}

internal Matrix4x4
matrix4x4_perspective(f32 fov_deg, f32 aspect_ratio, f32 z_near, f32 z_far) {
  // NOTE: Row Major
  Matrix4x4 result = make_matrix4x4(1.0f);
  f32 fov_rad = 1.0f / tan_f32(radians_pi32(fov_deg*0.5f));
  result.buf[0][0] = fov_rad*aspect_ratio;
  result.buf[1][1] = fov_rad;
  result.buf[2][2] = z_far / (z_far - z_near);
  result.buf[2][3] = 1.0f;
  result.buf[3][2] = (z_far*z_near) / (z_far - z_near);
  result.buf[3][3] = 0.0f;
  return(result);
}

internal Matrix4x4
matrix4x4_scale(Vector3 scale) {
  Matrix4x4 result = make_matrix4x4(1.0f);
  result.buf[0][0] = scale.x;
  result.buf[1][1] = scale.y;
  result.buf[2][2] = scale.z;
  return(result);
}

internal Matrix4x4
matrix4x4_translate(f32 x, f32 y, f32 z) {
  Matrix4x4 result = make_matrix4x4(1.0f);
  result.buf[3][0] = x;
  result.buf[3][1] = y;
  result.buf[3][2] = z;
  return(result);
}

internal Matrix4x4
matrix4x4_rotate(Vector3 axis, f32 t) {
  Matrix4x4 result = make_matrix4x4(1.0f);
  axis = vector3_normalize(axis);
  f32 sin_theta = sin_f32(t);
  f32 cos_theta = cos_f32(t);
  f32 cos_value = 1.0f - cos_theta;
  result.buf[0][0] = (axis.x*axis.x*cos_value) + cos_theta;
  result.buf[0][1] = (axis.x*axis.y*cos_value) + (axis.z*sin_theta);
  result.buf[0][2] = (axis.x*axis.z*cos_value) - (axis.y*sin_theta);
  result.buf[1][0] = (axis.y*axis.x*cos_value) - (axis.z*sin_theta);
  result.buf[1][1] = (axis.y*axis.y*cos_value) + cos_theta;
  result.buf[1][2] = (axis.y*axis.z*cos_value) + (axis.x*sin_theta);
  result.buf[2][0] = (axis.z*axis.x*cos_value) + (axis.y*sin_theta);
  result.buf[2][1] = (axis.z*axis.y*cos_value) - (axis.x*sin_theta);
  result.buf[2][2] = (axis.z*axis.z*cos_value) + cos_theta;
  return(result);
}

internal Matrix4x4
matrix4x4_mul(Matrix4x4 a, Matrix4x4 b) {
  Matrix4x4 result = {0};
  for (u32 j = 0; j < 4; ++j) {
    for (u32 i = 0; i < 4; ++i) {
      result.buf[i][j] = (a.buf[0][j]*b.buf[i][0] +
        a.buf[1][j]*b.buf[i][1] +
        a.buf[2][j]*b.buf[i][2] +
        a.buf[3][j]*b.buf[i][3]);
    }
  }
  return(result);
}

internal Vector4
matrix4x4_mul_vector4(Matrix4x4 m, Vector4 v) {
  Vector4 result = {
    .x = v.x*m.buf[0][0] + v.y*m.buf[1][0] + v.z*m.buf[2][0] + m.buf[3][0],
    .y = v.x*m.buf[0][1] + v.y*m.buf[1][1] + v.z*m.buf[2][1] + m.buf[3][1],
    .z = v.x*m.buf[0][2] + v.y*m.buf[1][2] + v.z*m.buf[2][2] + m.buf[3][2],
    .w = v.x*m.buf[0][3] + v.y*m.buf[1][3] + v.z*m.buf[2][3] + m.buf[3][3],
  };
  return(result);
}

#endif // KRUEGER_BASE_MATH_C
