#include "krueger_base.h"
#include "krueger_platform.h"
#include "krueger_shared.h"

#include "krueger_base.c"
#include "krueger_platform.c"

//////////////////////
// NOTE: Helper Macros

#define pack_rgba(r, g, b, a) ((a << 24) | (r << 16) | (g << 8) | (b << 0))

/////////////////
// NOTE: Load BMP

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
load_bmp(char *filepath, Arena *perm_arena, Arena *temp_arena) {
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
      u32 *pixels = arena_push(perm_arena, size);
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
    log_error("%s: failed to read file: %s", __func__, filepath);
  }
  temp_end(temp);
  return(result);
}

////////////////
// NOTE: Structs

typedef struct {
  u32 tile_w;
  u32 tile_h;
  u32 tile_count_x;
  u32 tile_count_y;
  u32 max_tile_count;
  Image image;
} Sprite_Sheet;

typedef struct {
  char *chars;
  Sprite_Sheet sheet;
} Font;

typedef struct {
  Arena perm_arena;
  Arena temp_arena;

  Image font_image;
  Sprite_Sheet font_sheet;
  Font font;
} Game_State;

/////////////////////////
// NOTE: Helper Functions

internal Sprite_Sheet
make_sprite_sheet(Image image,
                  u32 tile_w, u32 tile_h,
                  u32 tile_count_x, u32 tile_count_y) {
  Sprite_Sheet result = {
    .tile_w = tile_w,
    .tile_h = tile_h,
    .tile_count_x = tile_count_x,
    .tile_count_y = tile_count_y,
    .max_tile_count = tile_count_x*tile_count_y,
    .image = image,
  };
  return(result);
}

internal Image
get_sprite(Sprite_Sheet sheet, u32 tile_index) {
  assert(tile_index <= sheet.max_tile_count);
  u32 tile_w = sheet.tile_w;
  u32 tile_h = sheet.tile_h;
  u32 tile_x = tile_w*(tile_index%sheet.tile_count_x);
  u32 tile_y = tile_h*(tile_index/sheet.tile_count_x);
  Image result = make_subimage(sheet.image, tile_x, tile_y, tile_w, tile_h);
  return(result);
}

internal Font
make_font(char *chars, Sprite_Sheet sheet) {
  Font result = {
    .chars = chars,
    .sheet = sheet,
  };
  return(result);
}

///////////////////////
// NOTE: Draw Functions

internal void
clear(Image dst, u32 color) {
  for (u32 y = 0; y < dst.height; ++y) {
    for (u32 x = 0; x < dst.width; ++x) {
      dst.pixels[y*dst.pitch + x] = color;
    }
  }
}

internal void
draw_sprite(Image dst, Image src,
            s32 x, s32 y) {
  s32 min_x = clamp_bot(0, x);
  s32 min_y = clamp_bot(0, y);
  s32 max_x = clamp_top(x + src.width, dst.width);
  s32 max_y = clamp_top(y + src.height, dst.height);
  for (s32 dy = min_y; dy < max_y; ++dy) {
    s32 sy = dy - min_y;
    for (s32 dx = min_x; dx < max_x; ++dx) {
      s32 sx = dx - min_x;
      u32 dst_index = dy*dst.pitch + dx;
      u32 src_index = sy*src.pitch + sx;
      u32 src_color = src.pixels[src_index];
      if (src_color != 0xFFFF00FF) {
        dst.pixels[dst_index] = src_color;
      }
    }
  }
}

internal void
draw_text(Image dst, Font font,
          char *text, s32 x, s32 y) {
  s32 glyph_w = font.sheet.tile_w;
  s32 glyph_h = font.sheet.tile_h;
  s32 glyph_x = x;
  s32 glyph_y = y;
  u32 text_len = (u32)cstr_len(text);
  for (u32 i = 0; i < text_len; ++i) {
    char c = text[i];
    switch (c) {
      case '\n': {
        glyph_x = x;
        glyph_y += glyph_h;
      } continue;
      case '\t': {
        glyph_x += 2*glyph_w;
      } continue;
    }
    u32 char_index = (u32)cstr_index_of(font.chars, c);
    assert(font.chars[char_index] == c);
    Image sprite = get_sprite(font.sheet, char_index);
    draw_sprite(dst, sprite, glyph_x, glyph_y);
    glyph_x += glyph_w;
  }
}

///////////////////////
// NOTE: Game Functions

shared_function
KRUEGER_INIT_PROC(krueger_init) {
  assert(sizeof(Game_State) <= memory->memory_size);
  Game_State *state = (Game_State *)memory->memory_ptr;

  uxx half_memory_size = memory->memory_size/2;
  uxx perm_memory_size = half_memory_size - sizeof(Game_State);
  uxx temp_memory_size = half_memory_size;

  u8 *perm_memory_ptr = (u8 *)memory->memory_ptr + sizeof(Game_State);
  u8 *temp_memory_ptr = memory->memory_ptr + half_memory_size;

  state->perm_arena = make_arena(perm_memory_ptr, perm_memory_size);
  state->temp_arena = make_arena(temp_memory_ptr, temp_memory_size);
  
  char *chars = " !\"#$%&'()*+,-./"
                "0123456789:;<=>?"
                "@ABCDEFGHIJKLMNO"
                "PQRSTUVWXYZ[\\]^_"
                "                ";
  u32 num_chars_x = 16;
  u32 num_chars_y = 5;

  state->font_image = load_bmp("../res/font.bmp", &state->perm_arena, &state->temp_arena);
  state->font_sheet = make_sprite_sheet(state->font_image, 8, 8, num_chars_x, num_chars_y);
  state->font = make_font(chars, state->font_sheet);
}

shared_function
KRUEGER_FRAME_PROC(krueger_frame) {
  Game_State *state = (Game_State *)memory->memory_ptr;
  Image draw_buffer = *back_buffer;
  clear(draw_buffer, 0x202020);
  draw_text(draw_buffer, state->font, "KRUEGER", 0, 0);
}
