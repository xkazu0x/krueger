#include "krueger_base.h"
#include "krueger_platform.h"
#include "krueger_shared.h"

#include "krueger_base.c"
#include "krueger_platform.c"

#define mask_alpha(x) (((x) >> 24) & 0xFF)
#define mask_red(x)   (((x) >> 16) & 0xFF)
#define mask_green(x) (((x) >>  8) & 0xFF)
#define mask_blue(x)  (((x) >>  0) & 0xFF)
#define pack_rgba(r, g, b, a) ((a << 24) | (r << 16) | (g << 8) | (b << 0))

typedef struct {
  char *chars;
  u32 num_char_x;
  u32 num_char_y;
  u32 glyph_w;
  u32 glyph_h;
  Image image;
} Font;

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

internal Image
font_get_glyph(Font font, char c) {
  uxx char_index = cstr_index_of(font.chars, c);
  u32 tile_x = font.glyph_w*(char_index % font.num_char_x);
  u32 tile_y = font.glyph_h*((u32)floor_f32((f32)char_index/(f32)font.num_char_x));
  Image result = make_subimage(font.image, tile_x, tile_y, font.glyph_w, font.glyph_h);
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

internal Image
load_bmp(char *file_path, Arena *main_arena, Arena *temp_arena) {
  Image result = {0};
  Temp temp = temp_begin(temp_arena);
  void *file_data = platform_read_entire_file(temp.arena, file_path);
  if (file_data) {
    Bmp_File_Header *bmp_file = (Bmp_File_Header *)file_data;
    if (bmp_file->type == cstr_encode("BM")) {
      Bmp_Info_Header *bmp_info = (Bmp_Info_Header *)((u8 *)bmp_file + sizeof(Bmp_File_Header));

      u32 width = bmp_info->image_width;
      u32 height = bmp_info->image_height;
      u32 size = width*height*(bmp_info->bits_per_pixel/8);

      u32 *bmp_data = (u32 *)((u8 *)file_data + bmp_file->data_offset);
      u32 *pixels = arena_push(main_arena, size);

      // NOTE: Flip Vertically
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
          result.pixels[i] = pack_rgba(r, g, b, a);
        }
      }
    }
  } else {
    log_error("%s: failed to read file: %s", __func__, file_path);
  }
  temp_end(temp);
  return(result);
}

internal u32
color32_from_vector3(Vector3 color) {
  color.r = clamp(0.0f, color.r, 1.0f);
  color.g = clamp(0.0f, color.g, 1.0f);
  color.b = clamp(0.0f, color.b, 1.0f);
  u32 r = round_t(u32, color.r*255.0f);
  u32 g = round_t(u32, color.g*255.0f);
  u32 b = round_t(u32, color.b*255.0f);
  u32 a = 0xFF;
  u32 result = pack_rgba(r, g, b, a);
  return(result);
}

internal void
draw_clearv(Image image, Vector3 color) {
  u32 color32 = color32_from_vector3(color);
  for (u32 y = 0; y < image.height; ++y) {
    for (u32 x = 0; x < image.width; ++x) {
      image.pixels[y*image.pitch + x] = color32;
    }
  }
}

internal void
draw_rectv(Image image, Vector2 min, Vector2 max, Vector3 color) {
  if (min.x > max.x) swap_t(f32, min.x, max.x);
  if (min.y > max.y) swap_t(f32, min.y, max.y);
  s32 min_x = clamp_bot(0, round_t(s32, min.x));
  s32 min_y = clamp_bot(0, round_t(s32, min.y));
  s32 max_x = clamp_top(round_t(s32, max.x), (s32)image.width);
  s32 max_y = clamp_top(round_t(s32, max.y), (s32)image.height);
  u32 color32 = color32_from_vector3(color);
  for (s32 y = min_y; y < max_y; ++y) {
    for (s32 x = min_x; x < max_x; ++x) {
      image.pixels[y*image.pitch + x] = color32;
    }
  }
}

internal void
draw_texture(Image dst, Image src, s32 x, s32 y) {
  s32 min_x = clamp_bot(0, x);
  s32 min_y = clamp_bot(0, y);
  s32 max_x = clamp_top(x + src.width, dst.width);
  s32 max_y = clamp_top(y + src.height, dst.height);
  for (s32 dy = min_y; dy < max_y; ++dy) {
    s32 sy = dy - min_y;
    for (s32 dx = min_x; dx < max_x; ++dx) {
      s32 sx = dx - min_x;
      dst.pixels[dy*dst.pitch + dx] = src.pixels[sy*src.pitch + sx];
    }
  }
}

internal void
draw_text(Image image, Font font, char *text, s32 x, s32 y, u32 color) {
  s32 gw = font.glyph_w;
  s32 gh = font.glyph_h;
  s32 gx = x;
  s32 gy = y;
  uxx text_len = cstr_len(text);
  for (uxx i = 0; i < text_len; ++i) {
    char c = text[i];
    switch (c) {
      case '\n': {
        gx = x;
        gy += gh;
      } continue;
      case '\t': {
        gx += 2*gw;
      } continue;
    }
    Image glyph = font_get_glyph(font, c);
    draw_texture(image, glyph, gx, gy);
    gx += gw;
  }
}

internal void
draw_debug_info(Image draw_buffer, Clock *time, Font font) {
  s32 x = 0, y = 0;
  u32 color = 0x79241f;

  f32 fps = million(1.0f)/time->dt_us;
  f32 ms = time->dt_us/thousand(1.0f);

  local char fps_str[256];
  local char ms_str[256];

  sprintf(fps_str, "%.2f FPS", fps);
  sprintf(ms_str, "%.2f MS", ms);
  
  x = draw_buffer.width - (s32)cstr_len(fps_str)*font.glyph_w;
  draw_text(draw_buffer, font, fps_str, x, y, color);

  x = draw_buffer.width - (s32)cstr_len(ms_str)*font.glyph_w;
  y += font.glyph_h;
  draw_text(draw_buffer, font, ms_str, x, y, color);
}

typedef struct {
  Arena main_arena;
  Arena temp_arena;
  
  Image font_image;
  Font font;

  f32 dim;
  f32 speed;
  Vector2 pos;
  Vector2 dpos;
} Krueger_State;

shared_function
KRUEGER_INIT_PROC(krueger_init) {
  assert(sizeof(Krueger_State) <= memory->permanent_memory_size);
  Krueger_State *state = (Krueger_State *)memory->permanent_memory_ptr;

  uxx main_arena_size = memory->permanent_memory_size - sizeof(Krueger_State);
  u8 *main_arena_ptr = (u8 *)memory->permanent_memory_ptr + sizeof(Krueger_State);

  state->main_arena = make_arena(main_arena_ptr, main_arena_size);
  state->temp_arena = make_arena(memory->transient_memory_ptr, memory->transient_memory_size);

  char *chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ "
                "0123456789.,!?'\"-+=/\\%()<> ";
  u32 num_char_max = (u32)cstr_len(chars);
  u32 num_char_x = num_char_max/2;
  u32 num_char_y = num_char_max/num_char_x;

  state->font_image = load_bmp("../res/font.bmp", &state->main_arena, &state->temp_arena);
  state->font = make_font(chars, num_char_x, num_char_y, state->font_image, 8, 8);

  state->dim = 16.0f;
  state->speed = 100.0f;
  state->pos = make_vector2(16.0f, 16.0f);
  state->dpos = make_vector2(0.0f, 0.0f);
}

shared_function
KRUEGER_FRAME_PROC(krueger_frame) {
  Krueger_State *state = (Krueger_State *)memory->permanent_memory_ptr;
  Digital_Button *kbd = input->kbd;

  if (kbd[KEY_UP].is_down)    state->dpos.y = -1.0f;
  if (kbd[KEY_LEFT].is_down)  state->dpos.x = -1.0f;
  if (kbd[KEY_DOWN].is_down)  state->dpos.y =  1.0f;
  if (kbd[KEY_RIGHT].is_down) state->dpos.x =  1.0f;
  
  state->dpos = vector2_mul(state->dpos, state->speed*time->dt);
  state->pos = vector2_add(state->pos, state->dpos);
  state->dpos = make_vector2(0.0f, 0.0f);

  Image draw_buffer = *back_buffer;
  draw_clearv(draw_buffer, make_vector3(0.2f, 0.3f, 0.3f));
  
  Vector2 half_dim = make_vector2(state->dim/2, state->dim/2);
  Vector2 min = vector2_sub(state->pos, half_dim);
  Vector2 max = vector2_add(state->pos, half_dim);
  Vector3 color = make_vector3(1.0f, 0.5f, 0.2f);
  draw_rectv(draw_buffer, min, max, color);
  
  draw_debug_info(draw_buffer, time, state->font);
}
