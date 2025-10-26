#include "krueger_base.h"
#include "krueger_platform.h"
#include "krueger_shared.h"
#include "krueger.h"
#include "krueger_gfx.h"

#include "krueger_base.c"
#include "krueger_platform.c"
#include "krueger_gfx.c"

internal Font
make_font(char *chars, u32 num_char_x, u32 num_char_y, 
          Image image, u32 glyph_w, u32 glyph_h) {
  Font result = {
    .chars = chars,
    .num_char_x = num_char_x,
    .num_char_y = num_char_y,
    .glyph_w = glyph_w,
    .glyph_h = glyph_h,
    .image = image,
  };
  return(result);
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

internal b32
read_entire_file(Arena *arena, char *path, void** buffer, u64 *size) {
  b32 result = false;
  Platform_Handle file = platform_file_open(path, PLATFORM_FILE_READ | PLATFORM_FILE_SHARE_READ);
  if (!platform_handle_match(file, PLATFORM_HANDLE_NULL)) {
    *size = platform_get_file_size(file);
    *buffer = arena_push(arena, *size);
    u64 read_size = platform_file_read(file, *buffer, *size);
    if (read_size == *size) result = true;
    platform_file_close(file);
  }
  return(result);
}

internal Image
load_bmp(char *filepath, Arena *main_arena, Arena *temp_arena) {
  Image result = {0};
  u64 file_size;
  void *file_data;
  Temp temp = temp_begin(temp_arena);
  if (read_entire_file(temp.arena, filepath, &file_data, &file_size)) {
    Bmp_File_Header *bmp_file = (Bmp_File_Header *)file_data;
    if (bmp_file->type == cstr_encode("BM")) {
      Bmp_Info_Header *bmp_info = (Bmp_Info_Header *)((u8 *)bmp_file + sizeof(Bmp_File_Header));
      u32 width = bmp_info->image_width;
      u32 height = bmp_info->image_height;
      uxx size = width*height*(bmp_info->bits_per_pixel/8);
      u32 *bmp_data = (u32 *)((u8 *)file_data + bmp_file->data_offset);

      u32 *pixels = arena_push(main_arena, size);
      for (u32 y = 0; y < height; ++y) {
        for (u32 x = 0; x < width; ++x) {
          u32 color = bmp_data[width*(height-1) - y*width + x];
          pixels[y*width + x] = color;
        }
      }
      result = make_image(pixels, width, height);

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
    log_error("%s: failed to read file: %s", __func__, filepath);
  }
  temp_end(temp);
  return(result);
}

internal Mesh
load_obj(char *filepath, Arena *main_arena, Arena *temp_arena) {
  Mesh mesh = {0};
  u64 file_size;
  u8 *file_data;
  Temp temp = temp_begin(temp_arena);
  if (read_entire_file(temp.arena, filepath, &file_data, &file_size)) {
    u32 tmp_str_count = 0;
    String8 *tmp_strs = 0;

    u8 *stream = file_data;
    for (;*stream != 0; ++stream) {
      if (!char_is_space(*stream)) {
        String8 *str = arena_push_array(temp.arena, String8, 1);
        if (tmp_strs == 0) tmp_strs = str;
        str->str = stream;
        while (!char_is_space(*++stream));
        str->len = stream - str->str;
        ++tmp_str_count;
      }
    }

    u32 max_vertex_count = 0;
    u32 max_vertex_index_count = 0;

    for (u32 str_index = 0; str_index < tmp_str_count;) {
      String8 str = tmp_strs[str_index];
      if (str8_match_lit(str, "v")) {
        max_vertex_count += 1;
        str_index += 4;
      } else if (str8_match_lit(str, "f")) {
        max_vertex_index_count += 3;
        str_index += 4;
      } else {
        str_index += 1;
      }
    }

    mesh.vertices = arena_push_array(main_arena, Vector3, max_vertex_count);
    mesh.vertex_indices = arena_push_array(main_arena, u32, max_vertex_index_count);

    for (u32 str_index = 0; str_index < tmp_str_count;) {
      String8 *str = tmp_strs + str_index;
      if (str8_match_lit(*str, "v")) {
        Vector3 *vertex = mesh.vertices + mesh.vertex_count++;
        for (u32 i = 0; i < array_count(vertex->buf); ++i) {
          vertex->buf[i] = f32_from_str8(*++str);
        }
        str_index += 4;
      } else if (str8_match_lit(*str, "f")) {
        for (u32 i = 0; i < 3; ++i) {
          u32 index = mesh.vertex_index_count++;
          mesh.vertex_indices[index] = u32_from_str8(*++str);
        }
        str_index += 4;
      } else {
        str_index += 1;
      }
    }
  } else {
    log_error("%s: failed to open file: %s", __func__, filepath);
  }

  temp_end(temp);
  return(mesh);
}

internal Matrix4x4
matrix4x4_point_at(Vector3 eye, Vector3 center, Vector3 up) {
  Matrix4x4 result = make_matrix4x4(1.0f);
  center = vector3_normalize(vector3_sub(center, eye));
  up = vector3_normalize(vector3_sub(up, vector3_mul(center, vector3_dot(up, center))));
  Vector3 right = vector3_cross(up, center);
  result.buf[0][0] = right.x;
  result.buf[0][1] = right.y;
  result.buf[0][2] = right.z;
  result.buf[1][0] = up.x;
  result.buf[1][1] = up.y;
  result.buf[1][2] = up.z;
  result.buf[2][0] = center.x;
  result.buf[2][1] = center.y;
  result.buf[2][2] = center.z;
  result.buf[3][0] = eye.x;
  result.buf[3][1] = eye.y;
  result.buf[3][2] = eye.z;
  return(result);
}

internal Matrix4x4
matrix4x4_quick_inverse(Matrix4x4 m) {
  Matrix4x4 result = make_matrix4x4(1.0f);
  result.buf[0][0] = m.buf[0][0];
  result.buf[0][1] = m.buf[1][0];
  result.buf[0][2] = m.buf[2][0];
  result.buf[1][0] = m.buf[0][1];
  result.buf[1][1] = m.buf[1][1];
  result.buf[1][2] = m.buf[2][1];
  result.buf[2][0] = m.buf[0][2];
  result.buf[2][1] = m.buf[1][2];
  result.buf[2][2] = m.buf[2][2];
  result.buf[3][0] = -(m.buf[3][0]*result.buf[0][0] + 
                       m.buf[3][1]*result.buf[1][0] + 
                       m.buf[3][2]*result.buf[2][0]);
  result.buf[3][1] = -(m.buf[3][0]*result.buf[0][1] + 
                       m.buf[3][1]*result.buf[1][1] + 
                       m.buf[3][2]*result.buf[2][1]);
  result.buf[3][2] = -(m.buf[3][0]*result.buf[0][2] + 
                       m.buf[3][1]*result.buf[1][2] + 
                       m.buf[3][2]*result.buf[2][2]);
  return(result);
}

internal Vector2
project_point_to_screen(Vector2 p, u32 w, u32 h) {
  Vector2 result = {0};
  result.x = (p.x + 1.0f)*0.5f*w;
  result.y = (-p.y + 1.0f)*0.5f*h;
  return(result);
}

internal void
test_mesh(Image back_buffer, Mesh mesh, Krueger_State *state, u32 width, u32 height) {
  f32 aspect_ratio = (f32)height/(f32)width;
  Matrix4x4 proj = matrix4x4_perspective(90.0f, aspect_ratio, 0.1f, 100.0f);

  Vector3 cam_target = make_vector3(0.0f, 0.0f, 1.0f);
  Matrix4x4 cam_rotate = matrix4x4_rotate(make_vector3(0.0f, 1.0f, 0.0f), radians_f32(state->cam_yaw));
  state->cam_dir = matrix4x4_mul_vector4(cam_rotate, vector4_from_vector3(cam_target, 1.0f)).xyz;
  cam_target = vector3_add(state->cam_p, state->cam_dir);
  Matrix4x4 view = matrix4x4_quick_inverse(matrix4x4_point_at(state->cam_p, cam_target, state->cam_up));

  Matrix4x4 scale = matrix4x4_scale(make_vector3(1.0f, 1.0f, 1.0f));
  Matrix4x4 rotate = matrix4x4_rotate(make_vector3(1.0f, 1.0f, 0.0f), radians_f32(state->mesh_rot_angle));
  Matrix4x4 translate = matrix4x4_translate(0.0f, 0.0f, 4.0f);

  Matrix4x4 model = make_matrix4x4(1.0f);
  model = matrix4x4_mul(scale, model);
  model = matrix4x4_mul(rotate, model);
  model = matrix4x4_mul(translate, model);

  for (u32 i = 0; i < mesh.vertex_index_count; i += 3) {
    u32 i0 = mesh.vertex_indices[i+0] - 1;
    u32 i1 = mesh.vertex_indices[i+1] - 1;
    u32 i2 = mesh.vertex_indices[i+2] - 1;

    Vector4 v0 = vector4_from_vector3(mesh.vertices[i0], 1.0f);
    Vector4 v1 = vector4_from_vector3(mesh.vertices[i1], 1.0f);
    Vector4 v2 = vector4_from_vector3(mesh.vertices[i2], 1.0f);

    v0 = matrix4x4_mul_vector4(model, v0);
    v1 = matrix4x4_mul_vector4(model, v1);
    v2 = matrix4x4_mul_vector4(model, v2);

    Vector3 d01 = vector3_sub(v1.xyz, v0.xyz);
    Vector3 d02 = vector3_sub(v2.xyz, v0.xyz);

    Vector3 normal = vector3_normalize(vector3_cross(d01, d02));
    Vector3 cam_ray = vector3_sub(v0.xyz, state->cam_p);

    f32 scalar = vector3_dot(normal, cam_ray);
    if (scalar < 0.0f) {
#if 0
      Vector3 light_dir = vector3_normalize(make_vector3(0.0f, 0.0f, -1.0f));
      f32 dp = vector3_dot(normal, light_dir);
      f32 ct = (dp + 1.0f)/2.0f;
      u8 r = (u8)(0xc1*ct);
      u8 g = (u8)(0xc1*ct);
      u8 b = (u8)(0xc1*ct);
      u32 color = (r << 16) | (g << 8) | (b << 0);
#else
      u32 color = 0xc1c1c1;
#endif

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

      s32 x0 = (s32)v0.x;
      s32 y0 = (s32)v0.y;
      s32 x1 = (s32)v1.x;
      s32 y1 = (s32)v1.y;
      s32 x2 = (s32)v2.x;
      s32 y2 = (s32)v2.y;

      draw_triangle(back_buffer, x0, y0, x1, y1, x2, y2, color);

#if 1
      draw_line(back_buffer, x0, y0, x1, y1, 0x79241f);
      draw_line(back_buffer, x1, y1, x2, y2, 0x79241f);
      draw_line(back_buffer, x2, y2, x0, y0, 0x79241f);
#endif
    }
  }
}

internal void
test_rect(Image back_buffer, Clock time) {
  f32 t = abs_t(f32, sin_f32(time.sec));
  s32 x = 0;
  s32 y = 0;
  s32 w = 20;
  s32 h = 20;
  s32 offset_x = 0;
  s32 offset_y = back_buffer.height - h*2;
  draw_rect(back_buffer, 
            offset_x + x, offset_y + y, 
            (s32)(offset_x + (t*(x + w))), (s32)(offset_y + (t*(y + h))), 
            0xc1c1c1);
  offset_x = w;
  draw_rect_f32(back_buffer, 
                (f32)(offset_x + x), (f32)(offset_y + y), 
                offset_x + (t*(x + w)), offset_y + (t*(y + h)), 
                0x79241f);
}

internal void
test_circle(Image back_buffer, Clock time) {
  f32 circle_rt = 10.0f;
  f32 circle_r = abs_t(f32, circle_rt*sin_f32(time.sec));
  draw_circle(back_buffer, 
              (s32)circle_rt, back_buffer.height - (s32)circle_rt, 
              (s32)circle_r, 0xc1c1c1);
  draw_circle_f32(back_buffer, 
                  3.0f*circle_rt, back_buffer.height - circle_rt, 
                  circle_r, 0x79241f);
}

internal void
test_triangle(Image back_buffer, Clock time) {
  Vector3 v[] = {
    {{ -0.5f, -0.5f, 0.0f }},
    {{ -0.5f,  0.5f, 0.0f }},
    {{  0.5f, -0.5f, 0.0f }},

    {{ -0.5f,  0.5f, 0.0f }},
    {{  0.5f,  0.5f, 0.0f }},
    {{  0.5f, -0.5f, 0.0f }},
  };

  u32 c[] = {
    0xc1c1c1,
    0x79241f,
  };
  
  f32 st = abs_t(f32, sin_f32(time.sec)*0.2f);

  Matrix4x4 scale = matrix4x4_scale(make_vector3(st, st, st));
  Matrix4x4 translate = matrix4x4_translate(0.7f, -0.7f, 0.0f);

  Matrix4x4 model = make_matrix4x4(1.0f);
  model = matrix4x4_mul(scale, model);
  model = matrix4x4_mul(translate, model);

  for (uxx vi = 0, ci = 0; vi < array_count(v); vi += 3, ++ci) {
    uxx vi0 = vi;
    uxx vi1 = vi+1;
    uxx vi2 = vi+2;

    Vector4 v0 = vector4_from_vector3(v[vi0], 1.0f);
    Vector4 v1 = vector4_from_vector3(v[vi1], 1.0f);
    Vector4 v2 = vector4_from_vector3(v[vi2], 1.0f);

    v0 = matrix4x4_mul_vector4(model, v0);
    v1 = matrix4x4_mul_vector4(model, v1);
    v2 = matrix4x4_mul_vector4(model, v2);

    u32 w = back_buffer.width;
    u32 h = back_buffer.height;

    v0.xy = project_point_to_screen(v0.xy, w, h);
    v1.xy = project_point_to_screen(v1.xy, w, h);
    v2.xy = project_point_to_screen(v2.xy, w, h);

    s32 x0 = (s32)v0.x;
    s32 y0 = (s32)v0.y;
    s32 x1 = (s32)v1.x;
    s32 y1 = (s32)v1.y;
    s32 x2 = (s32)v2.x;
    s32 y2 = (s32)v2.y;

    draw_triangle(back_buffer, x0, y0, x1, y1, x2, y2, c[ci]);
  } 

  translate = matrix4x4_translate(0.9f, -0.7f, 0.0f);
  model = make_matrix4x4(1.0f);
  model = matrix4x4_mul(scale, model);
  model = matrix4x4_mul(translate, model);

  for (uxx vi = 0, ci = 0; vi < array_count(v); vi += 3, ++ci) {
    uxx vi0 = vi;
    uxx vi1 = vi+1;
    uxx vi2 = vi+2;

    Vector4 v0 = vector4_from_vector3(v[vi0], 1.0f);
    Vector4 v1 = vector4_from_vector3(v[vi1], 1.0f);
    Vector4 v2 = vector4_from_vector3(v[vi2], 1.0f);

    v0 = matrix4x4_mul_vector4(model, v0);
    v1 = matrix4x4_mul_vector4(model, v1);
    v2 = matrix4x4_mul_vector4(model, v2);

    u32 w = back_buffer.width;
    u32 h = back_buffer.height;

    v0.xy = project_point_to_screen(v0.xy, w, h);
    v1.xy = project_point_to_screen(v1.xy, w, h);
    v2.xy = project_point_to_screen(v2.xy, w, h);

    f32 x0 = v0.x;
    f32 y0 = v0.y;
    f32 x1 = v1.x;
    f32 y1 = v1.y;
    f32 x2 = v2.x;
    f32 y2 = v2.y;

    draw_triangle_f32(back_buffer, x0, y0, x1, y1, x2, y2, c[ci]);
  } 
}

internal void
test_triangle3(Image back_buffer, Clock time) {
  Vector3 v[] = {
    {{ -0.5f, -0.5f, 0.0f }},
    {{ -0.5f,  0.5f, 0.0f }},
    {{  0.5f, -0.5f, 0.0f }},

    {{ -0.5f,  0.5f, 0.0f }},
    {{  0.5f,  0.5f, 0.0f }},
    {{  0.5f, -0.5f, 0.0f }},
  };

  u32 c[] = {
    0xFF0000,
    0x00FF00,
    0x0000FF,

    0x00FFFF,
    0xFF00FF,
    0xFFFF00,
  };

  f32 st = abs_t(f32, sin_f32(time.sec)*0.2f);

  Matrix4x4 scale = matrix4x4_scale(make_vector3(st, st, st));
  Matrix4x4 translate = matrix4x4_translate(0.7f, -0.9f, 0.0f);

  Matrix4x4 model = make_matrix4x4(1.0f);
  model = matrix4x4_mul(scale, model);
  model = matrix4x4_mul(translate, model);

  for (uxx vi = 0, ci = 0; vi < array_count(v); vi += 3, ci += 3) {
    uxx vi0 = vi;
    uxx vi1 = vi+1;
    uxx vi2 = vi+2;

    Vector4 v0 = vector4_from_vector3(v[vi0], 1.0f);
    Vector4 v1 = vector4_from_vector3(v[vi1], 1.0f);
    Vector4 v2 = vector4_from_vector3(v[vi2], 1.0f);

    v0 = matrix4x4_mul_vector4(model, v0);
    v1 = matrix4x4_mul_vector4(model, v1);
    v2 = matrix4x4_mul_vector4(model, v2);

    u32 w = back_buffer.width;
    u32 h = back_buffer.height;

    v0.xy = project_point_to_screen(v0.xy, w, h);
    v1.xy = project_point_to_screen(v1.xy, w, h);
    v2.xy = project_point_to_screen(v2.xy, w, h);

    s32 x0 = (s32)v0.x;
    s32 y0 = (s32)v0.y;
    s32 x1 = (s32)v1.x;
    s32 y1 = (s32)v1.y;
    s32 x2 = (s32)v2.x;
    s32 y2 = (s32)v2.y;

    uxx ci0 = ci;
    uxx ci1 = ci+1;
    uxx ci2 = ci+2;

    u32 c0 = c[ci0];
    u32 c1 = c[ci1];
    u32 c2 = c[ci2];

    draw_triangle3c(back_buffer, x0, y0, x1, y1, x2, y2, c0, c1, c2);
  } 

  translate = matrix4x4_translate(0.9f, -0.9f, 0.0f);
  model = make_matrix4x4(1.0f);
  model = matrix4x4_mul(scale, model);
  model = matrix4x4_mul(translate, model);

  for (uxx vi = 0, ci = 0; vi < array_count(v); vi += 3, ci += 3) {
    uxx vi0 = vi;
    uxx vi1 = vi+1;
    uxx vi2 = vi+2;

    Vector4 v0 = vector4_from_vector3(v[vi0], 1.0f);
    Vector4 v1 = vector4_from_vector3(v[vi1], 1.0f);
    Vector4 v2 = vector4_from_vector3(v[vi2], 1.0f);

    v0 = matrix4x4_mul_vector4(model, v0);
    v1 = matrix4x4_mul_vector4(model, v1);
    v2 = matrix4x4_mul_vector4(model, v2);

    u32 w = back_buffer.width;
    u32 h = back_buffer.height;

    v0.xy = project_point_to_screen(v0.xy, w, h);
    v1.xy = project_point_to_screen(v1.xy, w, h);
    v2.xy = project_point_to_screen(v2.xy, w, h);

    f32 x0 = v0.x;
    f32 y0 = v0.y;
    f32 x1 = v1.x;
    f32 y1 = v1.y;
    f32 x2 = v2.x;
    f32 y2 = v2.y;

    uxx ci0 = ci;
    uxx ci1 = ci+1;
    uxx ci2 = ci+2;

    u32 c0 = c[ci0];
    u32 c1 = c[ci1];
    u32 c2 = c[ci2];

    draw_triangle3c_f32(back_buffer, x0, y0, x1, y1, x2, y2, c0, c1, c2);
  } 
}

internal void
test_texture(Image back_buffer, Image texture) {
  draw_texture(back_buffer, texture, 8, 8);
}

internal void
test_text(Image back_buffer, Font font) {
  s32 x = 8;
  s32 y = font.glyph_h*2;
  y += font.glyph_h;
  draw_text(back_buffer, "ABCDEFGHIJKLMNOPQRSTUVWXYZ", x,  y, font, 0xffc1c1c1);
  y += font.glyph_h;
  draw_text(back_buffer, "0123456789.,!?'\"-+=/\\%()<>", x, y, font, 0xaac1c1c1);
}

internal void
draw_debug_info(Image back_buffer, Clock time, Font font) {
  s32 x, y;

  f32 fps = million(1)/time.dt_us;
  f32 ms = time.dt_us/thousand(1);

  local char fps_str[256];
  local char ms_str[256];

  sprintf(fps_str, "%.2f FPS", fps);
  sprintf(ms_str, "%.2f MS", ms);

  x = back_buffer.width - (s32)cstr_len(fps_str)*font.glyph_w - font.glyph_w;
  y = font.glyph_h;
  draw_text(back_buffer, fps_str, x, y, font, 0xff79241f);

  x = back_buffer.width - (s32)cstr_len(ms_str)*font.glyph_w - font.glyph_w*2;
  y += font.glyph_h;
  draw_text(back_buffer, ms_str, x, y, font, 0xff79241f);
}

shared_function
KRUEGER_INIT_PROC(krueger_init) {
  assert(sizeof(Krueger_State) <= memory->permanent_memory_size);
  Krueger_State *state = (Krueger_State *)memory->permanent_memory_ptr;

  uxx main_arena_size = memory->permanent_memory_size - sizeof(Krueger_State);
  u8 *main_arena_ptr = (u8 *)memory->permanent_memory_ptr + sizeof(Krueger_State);

  state->main_arena = make_arena(main_arena_ptr, main_arena_size);
  state->temp_arena = make_arena(memory->transient_memory_ptr, memory->transient_memory_size);

  char *font_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ "
                     "0123456789.,!?'\"-+=/\\%()<> ";

  state->font_image = load_bmp("../res/font.bmp", &state->main_arena, &state->temp_arena);
  state->font = make_font(font_chars, 27, 2, state->font_image, 8, 8);

  state->mesh = load_obj("../res/monkey.obj", &state->main_arena, &state->temp_arena);
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
}

shared_function
KRUEGER_FRAME_PROC(krueger_frame) {
  Krueger_State *state = (Krueger_State *)memory->permanent_memory_ptr;

  Digital_Button *kbd = input->kbd;

  if (kbd[KEY_A].is_down) state->cam_vel = vector3_cross(state->cam_dir, state->cam_up);
  if (kbd[KEY_D].is_down) state->cam_vel = vector3_cross(state->cam_up, state->cam_dir);
  if (kbd[KEY_W].is_down) state->cam_vel = vector3_add(state->cam_vel, state->cam_dir);
  if (kbd[KEY_S].is_down) state->cam_vel = vector3_sub(state->cam_vel, state->cam_dir);

  if (kbd[KEY_H].is_down) state->cam_rot_vel = -1.0f;
  if (kbd[KEY_L].is_down) state->cam_rot_vel = 1.0f;

  state->cam_p = vector3_add(state->cam_p, vector3_mul(state->cam_vel, state->cam_speed*time->dt));
  state->cam_vel = make_vector3(0.0f, 0.0f, 0.0f);

  state->cam_yaw += state->cam_rot_speed*state->cam_rot_vel*time->dt;
  state->cam_rot_vel = 0.0f;

  state->mesh_rot_angle += state->mesh_rot_speed*time->dt;

  image_fill(*back_buffer, 0);

  test_mesh(*back_buffer, state->mesh, state, back_buffer->width, back_buffer->height);
  test_rect(*back_buffer, *time);
  test_circle(*back_buffer, *time);
  test_triangle(*back_buffer, *time);
  test_triangle3(*back_buffer, *time);
  test_texture(*back_buffer, state->font_image);
  test_text(*back_buffer, state->font);

  draw_debug_info(*back_buffer, *time, state->font);
}

// TODO:
// - Scaling/Rotating Texture
// - Triangle Texture Mapping
// - Depth Buffer
// - Clipping
