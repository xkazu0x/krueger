#include "krueger_base.h"
#include "krueger_platform.h"
#include "krueger_shared.h"

#include "krueger_base.c"
#include "krueger_platform.c"

#include <stdio.h>

#define ALPHA_MASK(x) (((x) >> 24) & 0xFF)
#define RED_MASK(x)   (((x) >> 16) & 0xFF)
#define GREEN_MASK(x) (((x) >>  8) & 0xFF)
#define BLUE_MASK(x)  (((x) >>  0) & 0xFF)

internal void
unpack_rgba32(u32 color, u8 *r, u8 *g, u8 *b, u8 *a) {
  *a = ((color >> 24) & 0xFF);
  *r = ((color >> 16) & 0xFF);
  *g = ((color >>  8) & 0xFF);
  *b = ((color >>  0) & 0xFF);
}

internal u32
pack_rgba32(u8 r, u8 g, u8 b, u8 a) {
  u32 color = ((a << 24) | (r << 16) | (g << 8) | (b << 0));
  return(color);
}

internal u32
alpha_linear_blend(u32 dst, u32 src) {
  u8 r0, g0, b0, a0;
  u8 r1, g1, b1, a1;
  unpack_rgba32(dst, &r0, &g0, &b0, &a0);
  unpack_rgba32(src, &r1, &g1, &b1, &a1);
  u8 r = (u8)(lerp_f32(r0, r1, a1/255.0f) + 0.5f);
  u8 g = (u8)(lerp_f32(g0, g1, a1/255.0f) + 0.5f);
  u8 b = (u8)(lerp_f32(b0, b1, a1/255.0f) + 0.5f);
  u8 a = a0;
  u32 result = pack_rgba32(r, g, b, a);
  return(result);
}

internal void
clear(Image image, u32 color) {
  for (u32 i = 0; i < (image.width*image.height); ++i) {
    image.pixels[i] = color;
  }
}

internal void
draw_rect(Image image,
          s32 x0, s32 y0, 
          s32 x1, s32 y1,
          u32 color) {
  if (x0 > x1) swap_t(s32, x0, x1);
  if (y0 > y1) swap_t(s32, y0, y1);
  s32 min_x = clamp_bot(0, x0);
  s32 min_y = clamp_bot(0, y0);
  s32 max_x = clamp_top(x1, (s32)image.width);
  s32 max_y = clamp_top(y1, (s32)image.height);
  for (s32 y = min_y; y < max_y; ++y) {
    for (s32 x = min_x; x < max_x; ++x) {
      image.pixels[y*image.width + x] = color;
    }
  }
}

internal void
draw_circle(Image image,
            s32 cx, s32 cy, s32 r,
            u32 color) {
  r = abs_t(s32, r);
  s32 min_x = clamp_bot(0, cx - r);
  s32 min_y = clamp_bot(0, cy - r);
  s32 max_x = clamp_top(cx + r, (s32)image.width);
  s32 max_y = clamp_top(cy + r, (s32)image.height);
  for (s32 y = min_y; y < max_y; ++y) {
    s32 dy = y - cy;
    for (s32 x = min_x; x < max_x; ++x) {
      s32 dx = x - cx;
      if (dx*dx + dy*dy < r*r) {
        image.pixels[y*image.width + x] = color;
      }
    }
  }
}

internal void
draw_circle_f32(Image image,
            f32 cx, f32 cy, f32 r,
            u32 color) {
  r = abs_t(f32, r);
  f32 rs = r*r;
  s32 min_x = (s32)clamp_bot(0, floor_f32(cx - r));
  s32 min_y = (s32)clamp_bot(0, floor_f32(cy - r));
  s32 max_x = (s32)clamp_top(ceil_f32(cx + r), image.width);
  s32 max_y = (s32)clamp_top(ceil_f32(cy + r), image.height);
  for (s32 y = min_y; y < max_y; ++y) {
    f32 dy = (y + 0.5f) - cy;
    for (s32 x = min_x; x < max_x; ++x) {
      f32 dx = (x + 0.5f) - cx;
      if (dx*dx + dy*dy < rs) {
        image.pixels[y*image.width + x] = color;
      }
    }
  }
}

internal void
draw_line(Image image,
          s32 x0, s32 y0, 
          s32 x1, s32 y1,
          u32 color) {
  if (abs_t(s32, x1 - x0) > abs_t(s32, y1 - y0)) {
    if (x0 > x1) {
      swap_t(s32, x0, x1);
      swap_t(s32, y0, y1);
    }
    s32 dx = x1 - x0;
    s32 dy = y1 - y0;
    s32 dir = (dy < 0) ? -1 : 1;
    dy *= dir;
    if (dx != 0) {
      s32 y = y0;
      s32 d = 2*dy - dx;
      for (s32 x = x0; x <= x1; ++x) {
        if ((y >= 0 && y < (s32)image.height) &&
            (x >= 0 && x < (s32)image.width)) {
          image.pixels[y*image.width + x] = color; 
        }
        if (d >= 0) {
          y += dir;
          d = d - 2*dx;
        }
        d = d + 2*dy;
      }
    }
  } else {
    if (y0 > y1) {
      swap_t(s32, x0, x1);
      swap_t(s32, y0, y1);
    }
    s32 dx = x1 - x0;
    s32 dy = y1 - y0;
    s32 dir = (dx < 0) ? -1 : 1;
    dx *= dir;
    if (dy != 0) {
      s32 x = x0;
      s32 d = 2*dx - dy;
      for (s32 y = y0; y <= y1; ++y) {
        if ((y >= 0 && y < (s32)image.height) &&
            (x >= 0 && x < (s32)image.width)) {
          image.pixels[y*image.width + x] = color; 
        }
        if (d >= 0) {
          x += dir;
          d = d - 2*dy;
        }
        d = d + 2*dx;
      }
    }
  }
}

internal void
draw_triangle(Image image,
              s32 x0, s32 y0,
              s32 x1, s32 y1,
              s32 x2, s32 y2,
              u32 color) {
  s32 min_x = clamp_bot(0, MIN(MIN(x0, x1), x2));
  s32 min_y = clamp_bot(0, MIN(MIN(y0, y1), y2));
  s32 max_x = clamp_top(MAX(MAX(x0, x1), x2), (s32)image.width);
  s32 max_y = clamp_top(MAX(MAX(y0, y1), y2), (s32)image.height);
  s32 x01 = x1 - x0;
  s32 y01 = y1 - y0;
  s32 x12 = x2 - x1;
  s32 y12 = y2 - y1;
  s32 x20 = x0 - x2;
  s32 y20 = y0 - y2;
  s32 bias0 = (((y01 == 0) && (x01 > 0)) || (y01 < 0)) ? 0 : -1;
  s32 bias1 = (((y12 == 0) && (x12 > 0)) || (y12 < 0)) ? 0 : -1;
  s32 bias2 = (((y20 == 0) && (x20 > 0)) || (y20 < 0)) ? 0 : -1;
  for (s32 y = min_y; y < max_y; ++y) {
    s32 dy0 = y - y0;
    s32 dy1 = y - y1;
    s32 dy2 = y - y2;
    for (s32 x = min_x; x < max_x; ++x) {
      s32 dx0 = x - x0;
      s32 dx1 = x - x1;
      s32 dx2 = x - x2;
      s32 w0 = x01*dy0 - y01*dx0 + bias0;
      s32 w1 = x12*dy1 - y12*dx1 + bias1;
      s32 w2 = x20*dy2 - y20*dx2 + bias2;
      if ((w0 >= 0) && (w1 >= 0) && (w2 >= 0)) {
        image.pixels[y*image.width + x] = color;
      }
    }
  }
}

internal void
draw_triangle_f32(Image image,
                  f32 x0, f32 y0,
                  f32 x1, f32 y1,
                  f32 x2, f32 y2,
                  u32 color) {
  s32 min_x = (s32)clamp_bot(0, floor_f32(MIN(MIN(x0, x1), x2)));
  s32 min_y = (s32)clamp_bot(0, floor_f32(MIN(MIN(y0, y1), y2)));
  s32 max_x = (s32)clamp_top(ceil_f32(MAX(MAX(x0, x1), x2)), image.width);
  s32 max_y = (s32)clamp_top(ceil_f32(MAX(MAX(y0, y1), y2)), image.height);
  f32 x01 = x1 - x0;
  f32 y01 = y1 - y0;
  f32 x12 = x2 - x1;
  f32 y12 = y2 - y1;
  f32 x20 = x0 - x2;
  f32 y20 = y0 - y2;
  f32 bias0 = (((y01 == 0.0f) && (x01 > 0.0f)) || (y01 < 0.0f)) ? 0.0f : -0.0001f;
  f32 bias1 = (((y12 == 0.0f) && (x12 > 0.0f)) || (y12 < 0.0f)) ? 0.0f : -0.0001f;
  f32 bias2 = (((y20 == 0.0f) && (x20 > 0.0f)) || (y20 < 0.0f)) ? 0.0f : -0.0001f;
  for (s32 y = min_y; y < max_y; ++y) {
    f32 dy0 = ((f32)y + 0.5f) - y0;
    f32 dy1 = ((f32)y + 0.5f) - y1;
    f32 dy2 = ((f32)y + 0.5f) - y2;
    for (s32 x = min_x; x < max_x; ++x) {
      f32 dx0 = ((f32)x + 0.5f) - x0;
      f32 dx1 = ((f32)x + 0.5f) - x1;
      f32 dx2 = ((f32)x + 0.5f) - x2;
      f32 w0 = x01*dy0 - y01*dx0 + bias0;
      f32 w1 = x12*dy1 - y12*dx1 + bias1;
      f32 w2 = x20*dy2 - y20*dx2 + bias2;
      if ((w0 >= 0.0f) && (w1 >= 0.0f) && (w2 >= 0.0f)) {
        image.pixels[y*image.width + x] = color;
      }
    }
  }
}

internal void
draw_triangle3(Image image,
               s32 x0, s32 y0, u32 c0,
               s32 x1, s32 y1, u32 c1,
               s32 x2, s32 y2, u32 c2) {
  s32 min_x = clamp_bot(0, MIN(MIN(x0, x1), x2));
  s32 min_y = clamp_bot(0, MIN(MIN(y0, y1), y2));
  s32 max_x = clamp_top(MAX(MAX(x0, x1), x2), (s32)image.width);
  s32 max_y = clamp_top(MAX(MAX(y0, y1), y2), (s32)image.height);
  s32 x01 = x1 - x0;
  s32 y01 = y1 - y0;
  s32 x02 = x2 - x0;
  s32 y02 = y2 - y0;
  s32 x12 = x2 - x1;
  s32 y12 = y2 - y1;
  s32 x20 = x0 - x2;
  s32 y20 = y0 - y2;
  f32 det = (f32)(x01*y02 - y01*x02);
  s32 bias0 = (((y01 == 0) && (x01 > 0)) || (y01 < 0)) ? 0 : -1;
  s32 bias1 = (((y12 == 0) && (x12 > 0)) || (y12 < 0)) ? 0 : -1;
  s32 bias2 = (((y20 == 0) && (x20 > 0)) || (y20 < 0)) ? 0 : -1;
  for (s32 y = min_y; y < max_y; ++y) {
    s32 dy0 = y - y0;
    s32 dy1 = y - y1;
    s32 dy2 = y - y2;
    for (s32 x = min_x; x < max_x; ++x) {
      s32 dx0 = x - x0;
      s32 dx1 = x - x1;
      s32 dx2 = x - x2;
      f32 w0 = (f32)(x01*dy0 - y01*dx0 + bias0);
      f32 w1 = (f32)(x12*dy1 - y12*dx1 + bias1);
      f32 w2 = (f32)(x20*dy2 - y20*dx2 + bias2);
      if ((w0 >= 0.0f) && (w1 >= 0.0f) && (w2 >= 0.0f)) {
        f32 alpha = w0/det;
        f32 beta = w1/det;
        f32 gamma = w2/det;
        u8 r = (u8)(RED_MASK(c0)*alpha + RED_MASK(c1)*beta + RED_MASK(c2)*gamma);
        u8 g = (u8)(GREEN_MASK(c0)*alpha + GREEN_MASK(c1)*beta + GREEN_MASK(c2)*gamma);
        u8 b = (u8)(BLUE_MASK(c0)*alpha + BLUE_MASK(c1)*beta + BLUE_MASK(c2)*gamma);
        u8 a = 0xFF;
        u32 color = pack_rgba32(r, g, b, a);
        image.pixels[y*image.width + x] = color;
      }
    }
  }
}

internal void
draw_triangle3_f32(Image image,
                   f32 x0, f32 y0, u32 c0,
                   f32 x1, f32 y1, u32 c1,
                   f32 x2, f32 y2, u32 c2) {
  s32 min_x = (s32)clamp_bot(0, floor_f32(MIN(MIN(x0, x1), x2)));
  s32 min_y = (s32)clamp_bot(0, floor_f32(MIN(MIN(y0, y1), y2)));
  s32 max_x = (s32)clamp_top(ceil_f32(MAX(MAX(x0, x1), x2)), image.width);
  s32 max_y = (s32)clamp_top(ceil_f32(MAX(MAX(y0, y1), y2)), image.height);
  f32 x01 = x1 - x0;
  f32 y01 = y1 - y0;
  f32 x02 = x2 - x0;
  f32 y02 = y2 - y0;
  f32 x12 = x2 - x1;
  f32 y12 = y2 - y1;
  f32 x20 = x0 - x2;
  f32 y20 = y0 - y2;
  f32 det = x01*y02 - y01*x02;
  f32 bias0 = (((y01 == 0.0f) && (x01 > 0.0f)) || (y01 < 0.0f)) ? 0.0f : -0.0001f;
  f32 bias1 = (((y12 == 0.0f) && (x12 > 0.0f)) || (y12 < 0.0f)) ? 0.0f : -0.0001f;
  f32 bias2 = (((y20 == 0.0f) && (x20 > 0.0f)) || (y20 < 0.0f)) ? 0.0f : -0.0001f;
  for (s32 y = min_y; y < max_y; ++y) {
    f32 dy0 = ((f32)y + 0.5f) - y0;
    f32 dy1 = ((f32)y + 0.5f) - y1;
    f32 dy2 = ((f32)y + 0.5f) - y2;
    for (s32 x = min_x; x < max_x; ++x) {
      f32 dx0 = ((f32)x + 0.5f) - x0;
      f32 dx1 = ((f32)x + 0.5f) - x1;
      f32 dx2 = ((f32)x + 0.5f) - x2;
      f32 w0 = x01*dy0 - y01*dx0 + bias0;
      f32 w1 = x12*dy1 - y12*dx1 + bias1;
      f32 w2 = x20*dy2 - y20*dx2 + bias2;
      if ((w0 >= 0.0f) && (w1 >= 0.0f) && (w2 >= 0.0f)) {
        f32 alpha = w0/det;
        f32 beta = w1/det;
        f32 gamma = w2/det;
        u8 r = (u8)(RED_MASK(c0)*alpha + RED_MASK(c1)*beta + RED_MASK(c2)*gamma);
        u8 g = (u8)(GREEN_MASK(c0)*alpha + GREEN_MASK(c1)*beta + GREEN_MASK(c2)*gamma);
        u8 b = (u8)(BLUE_MASK(c0)*alpha + BLUE_MASK(c1)*beta + BLUE_MASK(c2)*gamma);
        u8 a = 0xFF;
        u32 color = pack_rgba32(r, g, b, a);
        image.pixels[y*image.width + x] = color;
      }
    }
  }
}

#pragma pack(push, 1)
typedef struct {
  u16 type;
  u32 size;
  u16 reserved1;
  u16 reserved2;
  u32 data_offset;
} Bmp_File_Header;

typedef struct {
  u32 header_size;
  s32 image_width;
  s32 image_height;
  u16 num_color_planes;
  u16 bits_per_pixel;
  u32 compression;
  u32 image_size;
  s32 x_resolution_ppm;
  s32 y_resolution_ppm;
  u32 num_colors;
  u32 num_important_colors;
} Bmp_Info_Header;

// NOTE: in big-endian
typedef struct {
  u32 red_mask;
  u32 green_mask;
  u32 blue_mask;
  u32 alpha_mask;
} Bmp_Color_Header;
#pragma pack(pop)

#if COMPILER_MSVC
#include <intrin.h>
#endif

internal b32
bit_scan_forward(u32 *index, u32 mask) {
  b32 result = false;
#if COMPILER_MSVC
  result = _BitScanForward((unsigned long *)index, mask);
#else
  for (u32 i = 0; i < 32; ++i) {
    if (mask & (1 << i)) {
      *index = i;
      result = true;
      break;
    }
  }
#endif
  return(result);
}

internal Image
load_bmp(char *filename) {
  Image result = {0};
  void *data = platform_read_file(filename);
  if (data) {
    Bmp_File_Header *bmp_file = (Bmp_File_Header *)data;
    if (bmp_file->type == cstr_encode("BM")) {
      Bmp_Info_Header *bmp_info = (Bmp_Info_Header *)((u8 *)bmp_file + sizeof(Bmp_File_Header));
      result.width = bmp_info->image_width;
      result.height = bmp_info->image_height;
      result.pixels = (u32 *)((u8 *)data + bmp_file->data_offset);
      if (bmp_info->header_size > 40) {
        Bmp_Color_Header *bmp_color = (Bmp_Color_Header *)((u8 *)bmp_info + sizeof(Bmp_Info_Header));
        u32 red_mask = bmp_color->red_mask;
        u32 green_mask = bmp_color->green_mask;
        u32 blue_mask = bmp_color->blue_mask;
        u32 alpha_mask = ~(red_mask | green_mask | blue_mask);
        u32 red_shift = 0;
        u32 green_shift = 0;
        u32 blue_shift = 0;
        u32 alpha_shift = 0;
        assert(bit_scan_forward(&red_shift, red_mask));
        assert(bit_scan_forward(&green_shift, green_mask));
        assert(bit_scan_forward(&blue_shift, blue_mask));
        assert(bit_scan_forward(&alpha_shift, alpha_mask));
        for (u32 i = 0; i < (result.width*result.height); ++i) {
          u32 color = result.pixels[i];
          u8 r = ((color >> red_shift) & 0xFF);
          u8 g = ((color >> green_shift) & 0xFF);
          u8 b = ((color >> blue_shift) & 0xFF);
          u8 a = ((color >> alpha_shift) & 0xFF);
          result.pixels[i] = pack_rgba32(r, g, b, a);
        }
      }
    }
  } else {
    printf("[ERROR]: load_bmp: failed to read file: %s\n", filename);
  }
  return(result);
}

typedef struct {
  Vector3 *vertex_buf;
  u32 *vertex_index_buf;
} Mesh;

internal Mesh
load_obj(char *filename) {
  Mesh mesh = {0};
  Vector3 *tmp_vertex_buf = 0;
  FILE *file = fopen(filename, "r");
  if (file) {
    char line[1<<8];
    while (fscanf(file, "%s", line) != EOF) {
      if (cstr_match(line, "v")) {
        Vector3 v;
        fscanf(file, "%f %f %f\n", &v.x, &v.y, &v.z);
        buf_push(tmp_vertex_buf, v);
      } else if (cstr_match(line, "f")) {
        s32 f[3];
        fscanf(file, "%d %d %d\n", &f[0], &f[1], &f[2]);
        buf_push(mesh.vertex_index_buf, f[0]);
        buf_push(mesh.vertex_index_buf, f[1]);
        buf_push(mesh.vertex_index_buf, f[2]);
      }
    }
    fclose(file);
    for (u32 i = 0; i < buf_len(mesh.vertex_index_buf); ++i) {
      u32 vertex_index = mesh.vertex_index_buf[i];
      Vector3 v = tmp_vertex_buf[vertex_index-1];
      buf_push(mesh.vertex_buf, v);
    }
    buf_free(tmp_vertex_buf);
  } else {
    printf("[ERROR]: Failed to open file: %s\n", filename);
  }
  return(mesh);
}

internal Matrix4x4
matrix4x4_point_at(Vector3 eye, Vector3 center, Vector3 up) {
  Matrix4x4 result = make_matrix4x4(1.0f);
  center = vector3_normalize(vector3_sub(center, eye));
  up = vector3_normalize(vector3_sub(up, vector3_mul(center, vector3_dot(up, center))));
  Vector3 right = vector3_cross(up, center);
  result.m[0][0] = right.x;
  result.m[0][1] = right.y;
  result.m[0][2] = right.z;
  result.m[1][0] = up.x;
  result.m[1][1] = up.y;
  result.m[1][2] = up.z;
  result.m[2][0] = center.x;
  result.m[2][1] = center.y;
  result.m[2][2] = center.z;
  result.m[3][0] = eye.x;
  result.m[3][1] = eye.y;
  result.m[3][2] = eye.z;
  return(result);
}

internal Matrix4x4
matrix4x4_quick_inverse(Matrix4x4 m) {
  Matrix4x4 result = make_matrix4x4(1.0f);
  result.m[0][0] = m.m[0][0];
  result.m[0][1] = m.m[1][0];
  result.m[0][2] = m.m[2][0];
  result.m[1][0] = m.m[0][1];
  result.m[1][1] = m.m[1][1];
  result.m[1][2] = m.m[2][1];
  result.m[2][0] = m.m[0][2];
  result.m[2][1] = m.m[1][2];
  result.m[2][2] = m.m[2][2];
  result.m[3][0] = -(m.m[3][0]*result.m[0][0] + m.m[3][1]*result.m[1][0] + m.m[3][2]*result.m[2][0]);
  result.m[3][1] = -(m.m[3][0]*result.m[0][1] + m.m[3][1]*result.m[1][1] + m.m[3][2]*result.m[2][1]);
  result.m[3][2] = -(m.m[3][0]*result.m[0][2] + m.m[3][1]*result.m[1][2] + m.m[3][2]*result.m[2][2]);
  return(result);
}

internal Vector2
project_point_to_screen(Vector2 p, u32 w, u32 h) {
  Vector2 result = {
    .x = (p.x + 1.0f)*0.5f*w,
    .y = (-p.y + 1.0f)*0.5f*h,
  };
  return(result);
}

internal void
test_draw_mesh(Image back_buffer, Mesh mesh,
               Matrix4x4 model, Matrix4x4 view, Matrix4x4 proj,
               Vector3 cam_p, b32 line) {
  for (u32 vertex_index = 0;
       vertex_index < buf_len(mesh.vertex_buf);
       vertex_index += 3) {
    Vector4 v0 = vector4_from_vector3(mesh.vertex_buf[vertex_index], 1.0f);
    Vector4 v1 = vector4_from_vector3(mesh.vertex_buf[vertex_index+1], 1.0f);
    Vector4 v2 = vector4_from_vector3(mesh.vertex_buf[vertex_index+2], 1.0f);

    v0 = matrix4x4_mul_vector4(model, v0);
    v1 = matrix4x4_mul_vector4(model, v1);
    v2 = matrix4x4_mul_vector4(model, v2);

    Vector3 d01 = vector3_sub(v1.xyz, v0.xyz);
    Vector3 d02 = vector3_sub(v2.xyz, v0.xyz);

    Vector3 normal = vector3_normalize(vector3_cross(d01, d02));
    Vector3 cam_ray = vector3_sub(v0.xyz, cam_p);

    f32 scalar = vector3_dot(normal, cam_ray);

    if (scalar < 0.0f) {
      v0 = matrix4x4_mul_vector4(view, v0);
      v1 = matrix4x4_mul_vector4(view, v1);
      v2 = matrix4x4_mul_vector4(view, v2);

      v0 = matrix4x4_mul_vector4(proj, v0);
      v1 = matrix4x4_mul_vector4(proj, v1);
      v2 = matrix4x4_mul_vector4(proj, v2);

      v0 = vector4_div(v0, v0.w);
      v1 = vector4_div(v1, v1.w);
      v2 = vector4_div(v2, v2.w);

      u32 w = back_buffer.width;
      u32 h = back_buffer.height;

      v0.xy = project_point_to_screen(v0.xy, w, h);
      v1.xy = project_point_to_screen(v1.xy, w, h);
      v2.xy = project_point_to_screen(v2.xy, w, h);

      draw_triangle3(back_buffer, 
                     (s32)v0.x, (s32)v0.y, 0xFF0000,
                     (s32)v1.x, (s32)v1.y, 0x00FF00,
                     (s32)v2.x, (s32)v2.y, 0x0000FF);

      if (line) {
        draw_line(back_buffer, (s32)v0.x, (s32)v0.y, (s32)v1.x, (s32)v1.y, 0x79241f);
        draw_line(back_buffer, (s32)v1.x, (s32)v1.y, (s32)v2.x, (s32)v2.y, 0x79241f);
        draw_line(back_buffer, (s32)v2.x, (s32)v2.y, (s32)v0.x, (s32)v0.y, 0x79241f);
      }
    }
  }
}

internal void
test_draw_mesh_f32(Image back_buffer, Mesh mesh, 
                   Matrix4x4 model, Matrix4x4 view, Matrix4x4 proj,
                   Vector3 cam_p, b32 line) {
  for (u32 vertex_index = 0;
       vertex_index < buf_len(mesh.vertex_buf);
       vertex_index += 3) {
    Vector4 v0 = vector4_from_vector3(mesh.vertex_buf[vertex_index], 1.0f);
    Vector4 v1 = vector4_from_vector3(mesh.vertex_buf[vertex_index+1], 1.0f);
    Vector4 v2 = vector4_from_vector3(mesh.vertex_buf[vertex_index+2], 1.0f);

    v0 = matrix4x4_mul_vector4(model, v0);
    v1 = matrix4x4_mul_vector4(model, v1);
    v2 = matrix4x4_mul_vector4(model, v2);

    Vector3 d01 = vector3_sub(v1.xyz, v0.xyz);
    Vector3 d02 = vector3_sub(v2.xyz, v0.xyz);

    Vector3 normal = vector3_normalize(vector3_cross(d01, d02));
    Vector3 cam_ray = vector3_sub(v0.xyz, cam_p);

    f32 scalar = vector3_dot(normal, cam_ray);

    if (scalar < 0.0f) {
      v0 = matrix4x4_mul_vector4(view, v0);
      v1 = matrix4x4_mul_vector4(view, v1);
      v2 = matrix4x4_mul_vector4(view, v2);

      v0 = matrix4x4_mul_vector4(proj, v0);
      v1 = matrix4x4_mul_vector4(proj, v1);
      v2 = matrix4x4_mul_vector4(proj, v2);

      v0 = vector4_div(v0, v0.w);
      v1 = vector4_div(v1, v1.w);
      v2 = vector4_div(v2, v2.w);

      u32 w = back_buffer.width;
      u32 h = back_buffer.height;

      v0.xy = project_point_to_screen(v0.xy, w, h);
      v1.xy = project_point_to_screen(v1.xy, w, h);
      v2.xy = project_point_to_screen(v2.xy, w, h);

      draw_triangle3_f32(back_buffer, 
                         v0.x, v0.y, 0x00FFFF,
                         v1.x, v1.y, 0xFF00FF,
                         v2.x, v2.y, 0xFFFF00);

      if (line) {
        draw_line(back_buffer, (s32)v0.x, (s32)v0.y, (s32)v1.x, (s32)v1.y, 0x79241f);
        draw_line(back_buffer, (s32)v1.x, (s32)v1.y, (s32)v2.x, (s32)v2.y, 0x79241f);
        draw_line(back_buffer, (s32)v2.x, (s32)v2.y, (s32)v0.x, (s32)v0.y, 0x79241f);
      }
    }
  }
}

internal void
draw_texture(u32 *dst_buf, u32 dst_w, u32 dst_h,
             u32 *src_buf, u32 src_w, u32 src_h,
             s32 x, s32 y) {
  s32 min_x = clamp_bot(0, x);
  s32 min_y = clamp_bot(0, y);
  s32 max_x = clamp_top(x + src_w, dst_w);
  s32 max_y = clamp_top(y + src_h, dst_h);
  for (s32 dy = min_y; dy < max_y; ++dy) {
    for (s32 dx = min_x; dx < max_x; ++dx) {
      u32 dst_index = dy*dst_w + dx;
      u32 src_index = src_w*(src_h-1) - dy*src_w + dx;
      u32 dst_c = dst_buf[dst_index];
      u32 src_c = src_buf[src_index];
      dst_buf[dst_index] = alpha_linear_blend(dst_c, src_c);
    }
  }
}

typedef struct {
  char *chars;
  u32 num_glyph_x;
  u32 num_glyph_y;
  u32 glyph_width;
  u32 glyph_height;
  Image image;
} Font;

internal Font
make_font(char *chars, 
          u32 num_glyph_x, u32 num_glyph_y, 
          u32 glyph_width, u32 glyph_height,
          Image image) {
  Font result = {0};
  result.chars = chars;
  result.num_glyph_x = num_glyph_x;
  result.num_glyph_y = num_glyph_y;
  result.glyph_width = glyph_width;
  result.glyph_height = glyph_height;
  result.image = image;
  return(result);
}

internal void
draw_char(Image image,
          char c, s32 x, s32 y, 
          Font font, Vector3 color) {
  s32 min_x = clamp_bot(0, x);
  s32 min_y = clamp_bot(0, y);
  s32 max_x = clamp_top(x + font.glyph_width, image.width);
  s32 max_y = clamp_top(y + font.glyph_height, image.height);
  uxx char_index = cstr_index_of(font.chars, c);
  uxx tile_x = char_index % font.num_glyph_x;
  uxx tile_y = (uxx)floor_f32((f32)char_index/font.num_glyph_x);
  u32 *glyph = font.image.pixels - tile_y*font.image.width*font.glyph_height + tile_x*font.glyph_width;
  for (s32 dy = min_y; dy < max_y; ++dy) {
    s32 gy = dy - min_y;
    for (s32 dx = min_x; dx < max_x; ++dx) {
      s32 gx = dx - min_x;
      s32 pixel_index = dy*image.width + dx;
      s32 glyph_index = font.image.width*(font.image.height-1) - gy*font.image.width + gx;
      u32 pixel_color = image.pixels[pixel_index];
      u32 glyph_color = glyph[glyph_index];
      f32 rt = clamp_top(color.r, 1.0f);
      f32 gt = clamp_top(color.g, 1.0f);
      f32 bt = clamp_top(color.b, 1.0f);
      u8 r = (u8)(rt*RED_MASK(glyph_color));
      u8 g = (u8)(gt*GREEN_MASK(glyph_color));
      u8 b = (u8)(bt*BLUE_MASK(glyph_color));
      u8 a = ALPHA_MASK(glyph_color);
      glyph_color = pack_rgba32(r, g, b, a);
      image.pixels[pixel_index] = alpha_linear_blend(pixel_color, glyph_color);
    }
  }
}

internal void
draw_text(Image image,
          char *text, s32 x, s32 y, 
          Font font, Vector3 color) {
  s32 gx = x;
  s32 gy = y;
  uxx text_len = cstr_len(text);
  for (uxx i = 0; i < text_len; ++i) {
    char c = text[i];
    switch (c) {
      case '\n': {
        gx = x;
        gy += font.glyph_height;
      } continue;
      case '\t': {
        gx += 2*font.glyph_width;
      } continue;
    }
    draw_char(image, c, gx, gy, font, color);
    gx += font.glyph_width;
  }
}

struct Krueger_State {
  Arena arena;

  Image font_image;
  Font font;

  Mesh mesh;
  f32 mesh_rot_angle;
  f32 mesh_rot_speed;

  Vector3 cam_p;
  Vector3 cam_up;
  Vector3 cam_dir;

  Vector3 cam_vel;
  f32 cam_speed;

  f32 cam_rot_vel;
  f32 cam_rot_speed;
  f32 cam_yaw;
};

shared_function
KRUEGER_INIT_PROC(krueger_init) {
  Arena arena = arena_alloc(MB(64));
  assert(sizeof(Krueger_State) <= arena.reserve_size);
  Krueger_State *state = push_array(&arena, Krueger_State, 1);
  state->arena = arena;

  state->font_image = load_bmp("../res/font.bmp");
  char *font_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ "
                     "0123456789.,!?'\"-+=/\\%()<> ";
  state->font = make_font(font_chars, 27, 2, 8, 8, state->font_image);

  state->mesh = load_obj("../res/monkey.obj");
  state->mesh_rot_angle = 0.0f;
  state->mesh_rot_speed = 100.0f;

  state->cam_p   = make_vector3(0.0f, 0.0f, 0.0f);
  state->cam_up  = make_vector3(0.0f, 1.0f, 0.0f);
  state->cam_dir = make_vector3(0.0f, 0.0f, 1.0f);

  state->cam_vel = make_vector3(0.0f, 0.0f, 0.0f);
  state->cam_speed = 10.0f;

  state->cam_rot_vel = 0.0f;
  state->cam_rot_speed = 120.0f;
  state->cam_yaw = 0.0f;

  return(state);
}

shared_function
KRUEGER_FRAME_PROC(krueger_frame) {
  Digital_Button *kbd = input.kbd;

  state->cam_vel = make_vector3(0.0f, 0.0f, 0.0f);
  if (kbd[KEY_A].is_down) state->cam_vel = vector3_cross(state->cam_dir, state->cam_up);
  if (kbd[KEY_D].is_down) state->cam_vel = vector3_cross(state->cam_up, state->cam_dir);
  if (kbd[KEY_W].is_down) state->cam_vel = vector3_add(state->cam_vel, state->cam_dir);
  if (kbd[KEY_S].is_down) state->cam_vel = vector3_sub(state->cam_vel, state->cam_dir);
  state->cam_p = vector3_add(state->cam_p, vector3_mul(state->cam_vel, state->cam_speed*time.dt_sec));
  
  state->cam_rot_vel = 0.0f;
  if (kbd[KEY_H].is_down) state->cam_rot_vel = -1.0f;
  if (kbd[KEY_L].is_down) state->cam_rot_vel = 1.0f;
  state->cam_yaw += state->cam_rot_speed*state->cam_rot_vel*time.dt_sec;

  state->mesh_rot_angle += state->mesh_rot_speed*time.dt_sec;

  f32 aspect_ratio = (f32)back_buffer.height/(f32)back_buffer.width;
  Matrix4x4 proj = matrix4x4_perspective(90.0f, aspect_ratio, 0.1f, 100.0f);

  Vector3 cam_target = make_vector3(0.0f, 0.0f, 1.0f);
  Matrix4x4 cam_rotate = matrix4x4_rotate(make_vector3(0.0f, 1.0f, 0.0f), radians_f32(state->cam_yaw));
  state->cam_dir = matrix4x4_mul_vector4(cam_rotate, vector4_from_vector3(cam_target, 1.0f)).xyz;
  cam_target = vector3_add(state->cam_p, state->cam_dir);
  Matrix4x4 view = matrix4x4_quick_inverse(matrix4x4_point_at(state->cam_p, cam_target, state->cam_up));

  clear(back_buffer, 0);

  Matrix4x4 scale = matrix4x4_scale(make_vector3(1.0f, 1.0f, 1.0f));
  Matrix4x4 rotate = matrix4x4_rotate(make_vector3(1.0f, 1.0f, 0.0f), radians_f32(state->mesh_rot_angle));
  Matrix4x4 translate = matrix4x4_translate(-2.0f, 0.0f, 4.0f);
  Matrix4x4 model = make_matrix4x4(1.0f);
  model = matrix4x4_mul(scale, model);
  model = matrix4x4_mul(rotate, model);
  model = matrix4x4_mul(translate, model);
  test_draw_mesh(back_buffer, state->mesh, model, view, proj, state->cam_p, false);

  translate = matrix4x4_translate(2.0f, 0.0f, 4.0f);
  model = make_matrix4x4(1.0f);
  model = matrix4x4_mul(scale, model);
  model = matrix4x4_mul(rotate, model);
  model = matrix4x4_mul(translate, model);
  test_draw_mesh_f32(back_buffer, state->mesh, model, view, proj, state->cam_p, false);

  f32 circle_rt = 10.0f;
  f32 circle_r = circle_rt*sin_f32(time.sec);
  draw_circle(back_buffer, 
              back_buffer.width/2 - (s32)circle_rt, back_buffer.height/2, 
              (s32)circle_r, 0x79241f);
  draw_circle_f32(back_buffer, 
                  (f32)(back_buffer.width/2) + circle_rt, (f32)(back_buffer.height/2), 
                  circle_r, 0x79241f);

  draw_text(back_buffer, "0123456789",         0,  0, state->font, make_vector3(0.75f, 0.75f, 0.75f));
  draw_text(back_buffer, "ABCDEFGHIJKLM",      0,  8, state->font, make_vector3(0.75f, 0.75f, 0.75f));
  draw_text(back_buffer, "NOPQRSTUVWXYZ",      0, 16, state->font, make_vector3(0.75f, 0.75f, 0.75f));
  draw_text(back_buffer, ".,!?'\"-+=/\\%()<>", 0, 24, state->font, make_vector3(0.75f, 0.75f, 0.75f));

#if 1
  // NOTE: Draw Debug Info
  local char fps_str[256];
  local char ms_str[256];
  f32 fps = million(1)/time.dt_us;
  f32 ms = time.dt_us/thousand(1);
  sprintf(fps_str, "%.2f FPS", fps);
  sprintf(ms_str, "%.2f MS", ms);

  s32 offset_x = back_buffer.width - (s32)cstr_len(fps_str)*state->font.glyph_width;
  // s32 offset_y = back_buffer.height - state->font.glyph_height*2;
  s32 offset_y = 0;
  draw_text(back_buffer, fps_str, offset_x, offset_y, state->font, make_vector3(0.45f, 0.1f, 0.1f));
  offset_x = back_buffer.width - (s32)cstr_len(ms_str)*state->font.glyph_width - state->font.glyph_width;
  offset_y = offset_y + state->font.glyph_height;
  draw_text(back_buffer, ms_str, offset_x, offset_y, state->font, make_vector3(0.45f, 0.1f, 0.1f));
#endif
}

// TODO:
// - Clipping
// - Texture Mapping
// - Depth Buffer
