#include "krueger_base.h"
#include "krueger_base.c"
#include "krueger_shared.h"
#include "krueger_font.h"

#include <stdio.h>

#define ALPHA_MASK(x) ((x >> 24) & 0xFF)
#define RED_MASK(x)   ((x >> 16) & 0xFF)
#define GREEN_MASK(x) ((x >>  8) & 0xFF)
#define BLUE_MASK(x)  ((x >>  0) & 0xFF)

#pragma pack(push, 1)
typedef struct {
  u16 file_type;
  u32 file_size;
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

internal void *
load_bmp(char *filename, u32 *width, u32 *height) {
  void *result = 0;
  FILE *file = fopen(filename, "rb");
  if (file) {
    fseek(file, 0, SEEK_END);
    uxx file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    void *file_data = malloc(file_size);
    if (fread(file_data, file_size, 1, file)) {
      Bmp_File_Header *bmp_header = (Bmp_File_Header *)file_data;
      if (bmp_header->file_type == cstr_encode("BM")) {
        Bmp_Info_Header *bmp_info = (Bmp_Info_Header *)((u8 *)file_data + sizeof(Bmp_File_Header));
        *width = bmp_info->image_width;
        *height = bmp_info->image_height;
        result = (u32 *)((u8 *)file_data + bmp_header->data_offset);
        // printf("header size: %d\n", bmp_info->header_size);
      }
    } else {
      printf("[ERROR]: load_bmp: failed to read file: %s", filename);
    }
  } else {
    printf("[ERROR]: load_bmp: failed to open file: %s", filename);
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

internal void
draw_rect(u32 *pixels, u32 width, u32 height,
          s32 x0, s32 y0, 
          s32 x1, s32 y1,
          u32 color) {
  if (x0 > x1) swap_t(s32, x0, x1);
  if (y0 > y1) swap_t(s32, y0, y1);
  s32 min_x = clamp_bot(0, x0);
  s32 min_y = clamp_bot(0, y0);
  s32 max_x = clamp_top(x1, (s32)width);
  s32 max_y = clamp_top(y1, (s32)height);
  for (s32 y = min_y; y < max_y; ++y) {
    for (s32 x = min_x; x < max_x; ++x) {
      pixels[y*width + x] = color;
    }
  }
}

internal void
draw_line(u32 *buffer, u32 width, u32 height,
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
        if ((y >= 0 && y < (s32)height) &&
            (x >= 0 && x < (s32)width)) {
          u32 *dst = buffer + y*width + x;
          *dst = color; 
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
        if ((y >= 0 && y < (s32)height) &&
            (x >= 0 && x < (s32)width)) {
          u32 *dst = buffer + y*width + x;
          *dst = color; 
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
draw_char(u32 *pixels, u32 width, u32 height, 
          char c, s32 x, s32 y, 
          u8 *glyphs, u32 glyph_width, u32 glyph_height, 
          u32 font_size, u32 color) {
  u8 *glyph = glyphs + c*glyph_width*glyph_height*sizeof(u8);
  for (u32 gy = 0; gy < glyph_height; ++gy) {
    s32 py = y + gy*font_size;
    for (u32 gx = 0; gx < glyph_width; ++gx) {
      s32 px = x + gx*font_size;
      if (glyph[gy*glyph_width + gx]) {
        draw_rect(pixels, width, height, px, py, px + font_size, py + font_size, color);
      }
    }
  }
}

internal void
draw_text(u32 *pixels, u32 width, u32 height, 
          char *text, s32 x, s32 y, 
          u8 *glyphs, u32 glyph_width, u32 glyph_height, 
          u32 font_size, u32 color) {
  s32 gx = x;
  s32 gy = y;
  uxx text_len = cstr_len(text);
  for (uxx i = 0; i < text_len; ++i) {
    char c = text[i];
    switch (c) {
      case '\n': {
        gx = x;
        gy += glyph_height*font_size;
      } continue;
      case '\t': {
        gx += 2*glyph_width*font_size;
      } continue;
    }
    draw_char(pixels, width, height, c, gx, gy, glyphs, glyph_width, glyph_height, font_size, color);
    gx += glyph_width*font_size;
  }
}

internal void
draw_triangle_s32(u32 *pixels, u32 width, u32 height,
                  s32 x0, s32 y0,
                  s32 x1, s32 y1,
                  s32 x2, s32 y2,
                  u32 color) {
  s32 min_x = clamp_bot(0, min(min(x0, x1), x2));
  s32 min_y = clamp_bot(0, min(min(y0, y1), y2));
  s32 max_x = clamp_top(max(max(x0, x1), x2), (s32)width);
  s32 max_y = clamp_top(max(max(y0, y1), y2), (s32)height);
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
        pixels[y*width + x] = color;
      }
    }
  }
}

internal void
draw_triangle_f32(u32 *pixels, u32 width, u32 height,
                  f32 x0, f32 y0,
                  f32 x1, f32 y1,
                  f32 x2, f32 y2,
                  u32 color) {
  s32 min_x = clamp_bot(0, floor_f32(min(min(x0, x1), x2)));
  s32 min_y = clamp_bot(0, floor_f32(min(min(y0, y1), y2)));
  s32 max_x = clamp_top(ceil_f32(max(max(x0, x1), x2)), width);
  s32 max_y = clamp_top(ceil_f32(max(max(y0, y1), y2)), height);
  f32 x01 = x1 - x0;
  f32 y01 = y1 - y0;
  f32 x12 = x2 - x1;
  f32 y12 = y2 - y1;
  f32 x20 = x0 - x2;
  f32 y20 = y0 - y2;
  f32 bias0 = (((y01 == 0.0f) && (x01 > 0.0f)) || (y01 < 0.0f)) ? 0.0f : -0.0001;
  f32 bias1 = (((y12 == 0.0f) && (x12 > 0.0f)) || (y12 < 0.0f)) ? 0.0f : -0.0001;
  f32 bias2 = (((y20 == 0.0f) && (x20 > 0.0f)) || (y20 < 0.0f)) ? 0.0f : -0.0001;
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
        pixels[y*width + x] = color;
      }
    }
  }
}

internal void
draw_triangle3_s32(u32 *pixels, u32 width, u32 height,
                   s32 x0, s32 y0, u32 c0,
                   s32 x1, s32 y1, u32 c1,
                   s32 x2, s32 y2, u32 c2) {
  s32 min_x = clamp_bot(0, min(min(x0, x1), x2));
  s32 min_y = clamp_bot(0, min(min(y0, y1), y2));
  s32 max_x = clamp_top(max(max(x0, x1), x2), (s32)width);
  s32 max_y = clamp_top(max(max(y0, y1), y2), (s32)height);
  s32 x01 = x1 - x0;
  s32 y01 = y1 - y0;
  s32 x02 = x2 - x0;
  s32 y02 = y2 - y0;
  s32 x12 = x2 - x1;
  s32 y12 = y2 - y1;
  s32 x20 = x0 - x2;
  s32 y20 = y0 - y2;
  f32 det = x01*y02 - y01*x02;
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
      f32 w0 = x01*dy0 - y01*dx0 + bias0;
      f32 w1 = x12*dy1 - y12*dx1 + bias1;
      f32 w2 = x20*dy2 - y20*dx2 + bias2;
      if ((w0 >= 0.0f) && (w1 >= 0.0f) && (w2 >= 0.0f)) {
        f32 alpha = w0/det;
        f32 beta = w1/det;
        f32 gamma = w2/det;
        u8 a = 0xFF;
        u8 r = RED_MASK(c0)*alpha + RED_MASK(c1)*beta + RED_MASK(c2)*gamma;
        u8 g = GREEN_MASK(c0)*alpha + GREEN_MASK(c1)*beta + GREEN_MASK(c2)*gamma;
        u8 b = BLUE_MASK(c0)*alpha + BLUE_MASK(c1)*beta + BLUE_MASK(c2)*gamma;
        u32 color = ((a << 24) | (r << 16) | (g << 8) | (b << 0));
        pixels[y*width + x] = color;
      }
    }
  }
}

internal void
draw_triangle3_f32(u32 *pixels, u32 width, u32 height,
                   f32 x0, f32 y0, u32 c0,
                   f32 x1, f32 y1, u32 c1,
                   f32 x2, f32 y2, u32 c2) {
  s32 min_x = clamp_bot(0, floor_f32(min(min(x0, x1), x2)));
  s32 min_y = clamp_bot(0, floor_f32(min(min(y0, y1), y2)));
  s32 max_x = clamp_top(ceil_f32(max(max(x0, x1), x2)), width);
  s32 max_y = clamp_top(ceil_f32(max(max(y0, y1), y2)), height);
  f32 x01 = x1 - x0;
  f32 y01 = y1 - y0;
  f32 x02 = x2 - x0;
  f32 y02 = y2 - y0;
  f32 x12 = x2 - x1;
  f32 y12 = y2 - y1;
  f32 x20 = x0 - x2;
  f32 y20 = y0 - y2;
  f32 det = x01*y02 - y01*x02;
  f32 bias0 = (((y01 == 0.0f) && (x01 > 0.0f)) || (y01 < 0.0f)) ? 0.0f : -0.0001;
  f32 bias1 = (((y12 == 0.0f) && (x12 > 0.0f)) || (y12 < 0.0f)) ? 0.0f : -0.0001;
  f32 bias2 = (((y20 == 0.0f) && (x20 > 0.0f)) || (y20 < 0.0f)) ? 0.0f : -0.0001;
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
        u8 a = 0xFF;
        u8 r = RED_MASK(c0)*alpha + RED_MASK(c1)*beta + RED_MASK(c2)*gamma;
        u8 g = GREEN_MASK(c0)*alpha + GREEN_MASK(c1)*beta + GREEN_MASK(c2)*gamma;
        u8 b = BLUE_MASK(c0)*alpha + BLUE_MASK(c1)*beta + BLUE_MASK(c2)*gamma;
        u32 color = ((a << 24) | (r << 16) | (g << 8) | (b << 0));
        pixels[y*width + x] = color;
      }
    }
  }
}

internal Vector2
project_point_to_screen(Vector2 p, f32 w, f32 h) {
  Vector2 result = {
    .x = (p.x + 1.0f)*(f32)w*0.5f,
    .y = (-p.y + 1.0f)*(f32)h*0.5f,
  };
  return(result);
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

internal void
test_draw_mesh_s32(Image back_buffer, Mesh mesh,
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
      u32 w = back_buffer.width;
      u32 h = back_buffer.height;
      u32 *px = back_buffer.pixels;

      v0 = matrix4x4_mul_vector4(view, v0);
      v1 = matrix4x4_mul_vector4(view, v1);
      v2 = matrix4x4_mul_vector4(view, v2);

      v0 = matrix4x4_mul_vector4(proj, v0);
      v1 = matrix4x4_mul_vector4(proj, v1);
      v2 = matrix4x4_mul_vector4(proj, v2);

      v0 = vector4_div(v0, v0.w);
      v1 = vector4_div(v1, v1.w);
      v2 = vector4_div(v2, v2.w);

      v0.xy = project_point_to_screen(v0.xy, w, h);
      v1.xy = project_point_to_screen(v1.xy, w, h);
      v2.xy = project_point_to_screen(v2.xy, w, h);

      draw_triangle3_s32(px, w, h, 
                         v0.x, v0.y, 0xFF0000,
                         v1.x, v1.y, 0x00FF00,
                         v2.x, v2.y, 0x0000FF);

      if (line) {
        draw_line(px, w, h, v0.x, v0.y, v1.x, v1.y, 0x79241f);
        draw_line(px, w, h, v1.x, v1.y, v2.x, v2.y, 0x79241f);
        draw_line(px, w, h, v2.x, v2.y, v0.x, v0.y, 0x79241f);
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
      u32 w = back_buffer.width;
      u32 h = back_buffer.height;
      u32 *px = back_buffer.pixels;

      v0 = matrix4x4_mul_vector4(view, v0);
      v1 = matrix4x4_mul_vector4(view, v1);
      v2 = matrix4x4_mul_vector4(view, v2);

      v0 = matrix4x4_mul_vector4(proj, v0);
      v1 = matrix4x4_mul_vector4(proj, v1);
      v2 = matrix4x4_mul_vector4(proj, v2);

      v0 = vector4_div(v0, v0.w);
      v1 = vector4_div(v1, v1.w);
      v2 = vector4_div(v2, v2.w);

      v0.xy = project_point_to_screen(v0.xy, w, h);
      v1.xy = project_point_to_screen(v1.xy, w, h);
      v2.xy = project_point_to_screen(v2.xy, w, h);
      
      draw_triangle3_f32(px, w, h, 
                         v0.x, v0.y, 0x00FFFF,
                         v1.x, v1.y, 0xFF00FF,
                         v2.x, v2.y, 0xFFFF00);

      if (line) {
        draw_line(px, w, h, v0.x, v0.y, v1.x, v1.y, 0x79241f);
        draw_line(px, w, h, v1.x, v1.y, v2.x, v2.y, 0x79241f);
        draw_line(px, w, h, v2.x, v2.y, v0.x, v0.y, 0x79241f);
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
      // dst_buf[dy*dst_w + dx] = src_buf[dy*src_w + dx];
      // NOTE: vertically flipped
      dst_buf[dy*dst_w + dx] = src_buf[src_w*(src_h-1) - dy*src_w + dx];
    }
  }
}

global b32 initialized;
global char debug_str[256];
global u32 tick;
global Image bmp;
global Mesh mesh;
global Vector3 cam_p;
global Vector3 cam_up;
global Vector3 cam_dir;
global Vector3 cam_vel;
global f32 cam_yaw;

UPDATE_AND_RENDER_PROC(update_and_render) {
  Digital_Button *kbd = input.kbd;
  if (!initialized) {
    bmp.pixels = load_bmp("../res/4x4.bmp", &bmp.width, &bmp.height);
    mesh    = load_obj("../res/monkey.obj");
    cam_p   = make_vector3(0.0f, 0.0f, 0.0f);
    cam_up  = make_vector3(0.0f, 1.0f, 0.0f);
    cam_dir = make_vector3(0.0f, 0.0f, 1.0f);
    initialized = true;
  }

  cam_vel = make_vector3(0.0f, 0.0f, 0.0f);
  if (kbd[KEY_H].is_down) cam_vel.x--;
  if (kbd[KEY_J].is_down) cam_vel = vector3_sub(cam_vel, cam_dir);
  if (kbd[KEY_K].is_down) cam_vel = vector3_add(cam_vel, cam_dir);
  if (kbd[KEY_L].is_down) cam_vel.x++;
  cam_p = vector3_add(cam_p, cam_vel);

  if (kbd[KEY_A].is_down) cam_yaw--;
  if (kbd[KEY_F].is_down) cam_yaw++;

  f32 aspect_ratio = (f32)back_buffer.height/(f32)back_buffer.width;
  Matrix4x4 proj = matrix4x4_perspective(90.0f, aspect_ratio, 0.1f, 100.0f);

  Vector3 cam_target = make_vector3(0.0f, 0.0f, 1.0f);
  Matrix4x4 cam_rotate = matrix4x4_rotate(make_vector3(0.0f, 1.0f, 0.0f), radians_f32(cam_yaw));
  cam_dir = matrix4x4_mul_vector4(cam_rotate, vector4_from_vector3(cam_target, 1.0f)).xyz;
  cam_target = vector3_add(cam_p, cam_dir);
  Matrix4x4 view = matrix4x4_quick_inverse(matrix4x4_point_at(cam_p, cam_target, cam_up));

  image_clear(back_buffer, 0x000000);
  Matrix4x4 scale = matrix4x4_scale(make_vector3(1.0f, 1.0f, 1.0f));
  Matrix4x4 rotate = matrix4x4_rotate(make_vector3(1.0f, 1.0f, 0.0f), radians_f32(tick));
  Matrix4x4 translate = matrix4x4_translate(-2.0f, 0.0f, 4.0f);
  Matrix4x4 model = make_matrix4x4(1.0f);
  model = matrix4x4_mul(scale, model);
  model = matrix4x4_mul(rotate, model);
  model = matrix4x4_mul(translate, model);
  test_draw_mesh_s32(back_buffer, mesh, model, view, proj, cam_p, false);
  translate = matrix4x4_translate(2.0f, 0.0f, 4.0f);
  model = make_matrix4x4(1.0f);
  model = matrix4x4_mul(scale, model);
  model = matrix4x4_mul(rotate, model);
  model = matrix4x4_mul(translate, model);
  test_draw_mesh_f32(back_buffer, mesh, model, view, proj, cam_p, false);

  { // NOTE: Draw Debug Info
    f32 ms_per_frame = (f32)clock_delta/(f32)million(1);
    f32 frames_per_sec = (f32)billion(1)/(f32)clock_delta;

    sprintf(debug_str, "%.2f FPS\n%.2f MS", frames_per_sec, ms_per_frame);

    u32 text_size = 1;
    u32 text_color = 0xc1c1c1;

    s32 x_offset = 0;
    s32 y_offset = 0;
    draw_text(back_buffer.pixels, back_buffer.width, back_buffer.height,
              debug_str, x_offset, y_offset,
              (u8 *)default_font_glyphs, KRUEGER_FONT_WIDTH, KRUEGER_FONT_HEIGHT,
              text_size, text_color);

    local b32 switch_text_color = false;
    if (!(tick%10)) switch_text_color = !switch_text_color;
    if (switch_text_color) text_color = 0x79241f;

    x_offset = 4*KRUEGER_FONT_HEIGHT*text_size;
    y_offset = 6*KRUEGER_FONT_HEIGHT*text_size;
    draw_text(back_buffer.pixels, back_buffer.width, back_buffer.height,
              "int\nmain(void) {\n\tkrueger_init();\n\treturn(0);\n}", x_offset, y_offset,
              (u8 *)default_font_glyphs, KRUEGER_FONT_WIDTH, KRUEGER_FONT_HEIGHT,
              text_size, text_color);
  } // NOTE: Draw Debug Info

  draw_texture(back_buffer.pixels, back_buffer.width, back_buffer.height,
               bmp.pixels, bmp.width, bmp.height, 
               0, 0);

  ++tick;
}
