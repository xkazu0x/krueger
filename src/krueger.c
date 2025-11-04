#include "krueger_base.h"
#include "krueger_platform.h"
#include "krueger_shared.h"

#include "krueger_base.c"
#include "krueger_platform.c"

#include <time.h>

// TODO:
// - replace random number generator
// - endless flame ship particles
// - explosion particles when killing enemies
// - more enemies
// - corred player diagonal movement
// - game start and game over screen

//////////////////////
// NOTE: Helper Macros

#define mask_alpha(col) (((col) >> 24) & 0xFF)
#define mask_red(col)   (((col) >> 16) & 0xFF)
#define mask_green(col) (((col) >>  8) & 0xFF)
#define mask_blue(col)  (((col) >>  0) & 0xFF)

#define pack_rgba(r, g, b, a) ((a << 24) | (r << 16) | (g << 8) | (b << 0))
#define pack_rgb(r, g, b) ((r << 16) | (g << 8) | (b << 0))

//////////////////////
// NOTE: Color Pallete

#define CP_BLACK        0xFF000000
#define CP_DARK_BLUE    0xFF1D2B53
#define CP_DARK_RED     0xFF7E2553
#define CP_DARK_GREEN   0xFF008751
#define CP_BROWN        0xFFAB5236
#define CP_DARK_BROWN   0xFF5F574F
#define CP_GRAY         0xFFC2C3C7
#define CP_WHITE        0xFFFFF1E8
#define CP_RED          0xFFFF004D
#define CP_ORANGE       0xFFFFA300
#define CP_YELLOW       0xFFFFEC27
#define CP_GREEN        0xFF00E436
#define CP_BLUE         0xFF29ADFF
#define CP_DARK_GRAY    0xFF83769C
#define CP_PINK         0xFFFF77A8
#define CP_LIGHT_ORANGE 0xFFFFCCAA

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
  b32 is_alive;
  u32 sprite_index;
  f32 radius;
  f32 speed;
  Vector2 size;
  Vector2 pos;
  Vector2 dpos;
} Entity;

typedef struct {
  Arena perm_arena;
  Arena temp_arena;
  Image font_image;
  Image sprites_image;
  Sprite_Sheet font_sheet;
  Sprite_Sheet sprite_sheet;

  Font font;
  f32 tile_size;

  u32 score;
  u32 hp;
  u32 hp_sprite_index;
  u32 flame_sprite_index;
  f32 max_r;
  f32 r;

  f32 sec;
  f32 shoot_t;
  f32 shoot_cd;
  f32 invul_t;
  f32 invul_cd;
  f32 flash_t;
  f32 flash_cd;

  Entity player;

  u32 sprite_index;
  u32 sprite_count_x;
  u32 sprite_count_y;

  u32 max_sprite_count;
  u32 *sprite_indices;
  Entity enemy;
  u32 enemy_max_hp;
  u32 enemy_hp;

  u32 next_bullet;
  u32 max_bullet_count;
  Entity *bullets;

  u32 max_star_count;
  Entity *stars;

  b32 draw_debug_info;
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
  u32 tile_x = sheet.tile_w*(tile_index%sheet.tile_count_x);
  u32 tile_y = sheet.tile_h*(tile_index/sheet.tile_count_x);
  Image result = make_subimage(sheet.image, tile_x, tile_y, tile_w, tile_h);
  return(result);
}

internal Image
get_spritex(Sprite_Sheet sheet, u32 sprite_index, u32 sprite_count_x, u32 sprite_count_y) {
  assert(sprite_index <= sheet.max_tile_count);
  u32 tile_w = sheet.tile_w*sprite_count_x;
  u32 tile_h = sheet.tile_h*sprite_count_y;
  u32 tile_x = sheet.tile_w*(sprite_index%sheet.tile_count_x);
  u32 tile_y = sheet.tile_h*(sprite_index/sheet.tile_count_x);
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
image_clear(Image dst, u32 color) {
  for (u32 y = 0; y < dst.height; ++y) {
    for (u32 x = 0; x < dst.width; ++x) {
      dst.pixels[y*dst.pitch + x] = color;
    }
  }
}

internal void
draw_rect_fill(Image dst,
               s32 x0, s32 y0, 
               s32 x1, s32 y1,
               u32 color) {
  if (x0 > x1) swap_t(s32, x0, x1);
  if (y0 > y1) swap_t(s32, y0, y1);
  s32 min_x = clamp_bot(0, x0);
  s32 min_y = clamp_bot(0, y0);
  s32 max_x = clamp_top(x1, (s32)dst.width);
  s32 max_y = clamp_top(y1, (s32)dst.height);
  for (s32 y = min_y; y < max_y; ++y) {
    for (s32 x = min_x; x < max_x; ++x) {
      dst.pixels[y*dst.pitch + x] = color;
    }
  }
}

internal void
draw_rect_fill_f32(Image dst,
                 f32 x0, f32 y0, 
                 f32 x1, f32 y1,
                 u32 color) {
  if (x0 > x1) swap_t(f32, x0, x1);
  if (y0 > y1) swap_t(f32, y0, y1);
  s32 min_x = (s32)clamp_bot(0, floor_f32(x0));
  s32 min_y = (s32)clamp_bot(0, floor_f32(y0));
  s32 max_x = (s32)clamp_top(ceil_f32(x1), dst.width);
  s32 max_y = (s32)clamp_top(ceil_f32(y1), dst.height);
  for (s32 y = min_y; y < max_y; ++y) {
    for (s32 x = min_x; x < max_x; ++x) {
      dst.pixels[y*dst.pitch + x] = color;
    }
  }
}

internal void
draw_rect_fill_vec(Image dst, Vector2 min, Vector2 max, u32 color) {
  draw_rect_fill_f32(dst, min.x, min.y, max.x, max.y, color);
}

internal void
draw_circle_line(Image dst,
                 s32 cx, s32 cy, s32 r,
                 u32 color) {
  r = abs_t(s32, r);
  s32 x = 0;
  s32 y = -r;
  s32 p = -r;
  while (x < -y) {
    if (p > 0) {
      y += 1;
      p += 2*(x + y) + 1;
    } else {
      p += 2*x + 1;
    }
    // TODO: this is so ugly
    s32 xu = cx + y;
    s32 xl = cx - x;
    s32 yu = cy + y;
    s32 yl = cy - x;
    s32 xr = cx + x;
    s32 xd = cx - y;
    s32 yd = cy - y;
    s32 yr = cy + x;
#define check(x, max) if (((x) >= 0) && ((x) < ((s32)(max))))
    check(yu, dst.height) {
      check(xr, dst.width) dst.pixels[yu*dst.pitch + xr] = color;
      check(xl, dst.width) dst.pixels[yu*dst.pitch + xl] = color;
    }
    check(yd, dst.height) {
      check(xr, dst.width) dst.pixels[yd*dst.pitch + xr] = color;
      check(xl, dst.width) dst.pixels[yd*dst.pitch + xl] = color;
    }
    check(yr, dst.height) {
      check(xu, dst.width) dst.pixels[yr*dst.pitch + xu] = color;
      check(xd, dst.width) dst.pixels[yr*dst.pitch + xd] = color;
    }
    check(yl, dst.height) {
      check(xu, dst.width) dst.pixels[yl*dst.pitch + xu] = color;
      check(xd, dst.width) dst.pixels[yl*dst.pitch + xd] = color;
    }
#undef check
    x += 1;
  }
}

internal void
draw_circle_fill(Image dst,
                 s32 cx, s32 cy, s32 r,
                 u32 color) {
  r = abs_t(s32, r);
  s32 min_x = clamp_bot(0, cx - r);
  s32 min_y = clamp_bot(0, cy - r);
  s32 max_x = clamp_top(cx + r, (s32)dst.width);
  s32 max_y = clamp_top(cy + r, (s32)dst.height);
  for (s32 y = min_y; y < max_y; ++y) {
    s32 dy = y - cy;
    for (s32 x = min_x; x < max_x; ++x) {
      s32 dx = x - cx;
      if (dx*dx + dy*dy < r*r) {
        dst.pixels[y*dst.pitch + x] = color;
      }
    }
  }
}

internal void
draw_circle_fill_f32(Image dst,
                     f32 cx, f32 cy, f32 r,
                     u32 color) {
  r = abs_t(f32, r);
  s32 min_x = (s32)clamp_bot(0, floor_f32(cx - r));
  s32 min_y = (s32)clamp_bot(0, floor_f32(cy - r));
  s32 max_x = (s32)clamp_top(ceil_f32(cx + r), dst.width);
  s32 max_y = (s32)clamp_top(ceil_f32(cy + r), dst.height);
  for (s32 y = min_y; y < max_y; ++y) {
    f32 dy = (y + 0.5f) - cy;
    for (s32 x = min_x; x < max_x; ++x) {
      f32 dx = (x + 0.5f) - cx;
      if (dx*dx + dy*dy < r*r) {
        dst.pixels[y*dst.pitch + x] = color;
      }
    }
  }
}

internal void
draw_circle_fill_vec(Image dst, Vector2 center, f32 r, u32 color) {
  draw_circle_fill_f32(dst, center.x, center.y, r, color);
}

internal void
draw_sprite(Image dst, Image src, s32 x, s32 y) {
  s32 src_x = 0;
  s32 src_y = 0;
  s32 min_x = x;
  s32 min_y = y;
  s32 max_x = x + src.width;
  s32 max_y = y + src.height;
  if (min_x < 0) { src_x = -min_x; min_x = 0; }
  if (min_y < 0) { src_y = -min_y; min_y = 0; }
  if (max_x > (s32)dst.width) max_x = dst.width;
  if (max_y > (s32)dst.height) max_y = dst.height;
  u32 *src_row = src.pixels + src_y*src.pitch + src_x;
  u32 *dst_row = dst.pixels + min_y*dst.pitch + min_x;
  for (s32 dy = min_y; dy < max_y; ++dy) {
    u32 *src_pixel = src_row;
    u32 *dst_pixel = dst_row;
    for (s32 dx = min_x; dx < max_x; ++dx) {
      u32 src_color = *src_pixel;
      if (src_color != 0xFFFF00FF) {
        *dst_pixel = src_color;
      }
      ++dst_pixel;
      ++src_pixel;
    }
    src_row += src.pitch;
    dst_row += dst.pitch;
  }
}

internal void
draw_sprite_f32(Image dst, Image src, f32 x, f32 y) {
  s32 src_x = 0;
  s32 src_y = 0;
  s32 min_x = round_t(s32, x);
  s32 min_y = round_t(s32, y);
  s32 max_x = round_t(s32, x + src.width);
  s32 max_y = round_t(s32, y + src.height);
  if (min_x < 0) { src_x = -min_x + 1; min_x = 0; }
  if (min_y < 0) { src_y = -min_y + 1; min_y = 0; }
  if (max_x > (s32)dst.width) max_x = dst.width;
  if (max_y > (s32)dst.height) max_y = dst.height;
  u32 *src_row = src.pixels + src_y*src.pitch + src_x;
  u32 *dst_row = dst.pixels + min_y*dst.pitch + min_x;
  for (s32 dy = min_y; dy < max_y; ++dy) {
    u32 *src_pixel = src_row;
    u32 *dst_pixel = dst_row;
    for (s32 dx = min_x; dx < max_x; ++dx) {
      u32 color = *src_pixel;
      if (color != 0xFFFF00FF) {
        *dst_pixel = color;
      }
      ++dst_pixel;
      ++src_pixel;
    }
    src_row += src.pitch;
    dst_row += dst.pitch;
  }
}

internal void
draw_sprite_vec(Image dst, Image src, Vector2 pos) {
  draw_sprite_f32(dst, src, pos.x, pos.y);
}

#define PIXEL_SHADER_PROC(x) u32 x(u32 dst_color, u32 src_color, void *user_data)
typedef PIXEL_SHADER_PROC(pixel_shader_proc);

PIXEL_SHADER_PROC(shader_color_blend) {
  u32 user_color = *(u32 *)user_data;
  f32 rt = mask_red(user_color)/255.0f;
  f32 gt = mask_green(user_color)/255.0f;
  f32 bt = mask_blue(user_color)/255.0f;
  u8 r = round_t(u8, rt*mask_red(src_color));
  u8 g = round_t(u8, gt*mask_green(src_color));
  u8 b = round_t(u8, bt*mask_blue(src_color));
  u32 result = pack_rgb(r, g, b);
  return(result);
}

PIXEL_SHADER_PROC(shader_turn_all_white) {
  u32 result = CP_WHITE;
  return(result);
}

internal void
_draw_sprite_shader(Image dst, Image src, f32 x, f32 y,
                    pixel_shader_proc *callback, void *user_data) {
  s32 src_x = 0;
  s32 src_y = 0;
  s32 min_x = round_t(s32, x);
  s32 min_y = round_t(s32, y);
  s32 max_x = round_t(s32, x + src.width);
  s32 max_y = round_t(s32, y + src.height);
  if (min_x < 0) { src_x = -min_x + 1; min_x = 0; }
  if (min_y < 0) { src_y = -min_y + 1; min_y = 0; }
  if (max_x > (s32)dst.width) max_x = dst.width;
  if (max_y > (s32)dst.height) max_y = dst.height;
  u32 *src_row = src.pixels + src_y*src.pitch + src_x;
  u32 *dst_row = dst.pixels + min_y*dst.pitch + min_x;
  for (s32 dy = min_y; dy < max_y; ++dy) {
    u32 *src_pixel = src_row;
    u32 *dst_pixel = dst_row;
    for (s32 dx = min_x; dx < max_x; ++dx) {
      u32 src_color = *src_pixel;
      u32 dst_color = *dst_pixel;
      if (src_color != 0xFFFF00FF) {
        if (callback) {
          *dst_pixel = callback(dst_color, src_color, user_data);
        } else {
          *dst_pixel = src_color;
        }
      }
      ++dst_pixel;
      ++src_pixel;
    }
    src_row += src.pitch;
    dst_row += dst.pitch;
  }
}

// TODO: is it a good idea to separate this function
// out of the draw call?
internal Vector2
align_position(Image image, Vector2 pos, Vector2 align) {
  align.x = align.x*image.width;
  align.y = align.y*image.height;
  pos = vector2_sub(pos, align);
  return(pos);
}

internal void
draw_sprite_shader(Image dst, Image src, Vector2 pos,
                   pixel_shader_proc *callback, void *user_data) {
  _draw_sprite_shader(dst, src, pos.x, pos.y, callback, user_data);
}

internal void
draw_text(Image dst, Font font, String8 text,
          s32 x, s32 y, u32 color) {
  s32 glyph_w = font.sheet.tile_w;
  s32 glyph_h = font.sheet.tile_h;
  s32 glyph_x = x;
  s32 glyph_y = y;
  for (uxx i = 0; i < text.len; ++i) {
    u8 c = text.str[i];
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
    Vector2 pos = make_vector2((f32)glyph_x, (f32)glyph_y);
    draw_sprite_shader(dst, sprite, pos, shader_color_blend, &color);
    glyph_x += glyph_w;
  }
}

internal void
draw_text_align(Image dst, Font font, String8 text,
                s32 x, s32 y, f32 align_x, f32 align_y,
                u32 color) {
  s32 len = (s32)text.len;
  x = x - (s32)floor_f32(align_x*(font.sheet.tile_w*len));
  y = y - (s32)floor_f32(align_y*font.sheet.tile_h);
  draw_text(dst, font, text, x, y, color);
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

  state->font_image = load_bmp("../res/pico8_font.bmp", &state->perm_arena, &state->temp_arena);
  state->sprites_image = load_bmp("../res/sprites.bmp", &state->perm_arena, &state->temp_arena);
  state->font_sheet = make_sprite_sheet(state->font_image, 4, 6, 16, 6);
  state->sprite_sheet = make_sprite_sheet(state->sprites_image, 8, 8, 16, 16);

  char *chars = " !\"#$%&'()*+,-./"
                "0123456789:;<=>?"
                "@ABCDEFGHIJKLMNO"
                "PQRSTUVWXYZ[\\]^_"
                "`abcdefghijklmno"
                "pqrstuvwxyz{|}~ ";
  state->font = make_font(chars, state->font_sheet);
  state->tile_size = 8.0f;

  state->score = 0;
  state->hp = 3;
  state->hp_sprite_index = 32;
  state->flame_sprite_index = 16;
  state->max_r = 7.0f;

  state->sec = 0.0f;
  state->shoot_t = 0.0f;
  state->shoot_cd = 0.12f;
  state->invul_t = 0.0f;
  state->invul_cd = 1.2f;
  state->flash_t = 0.0f;
  state->flash_cd = 0.05f;

  state->player.is_alive = true;
  state->player.sprite_index = 1;
  state->player.speed = 100.0f;
  state->player.size = make_vector2(state->tile_size, state->tile_size);
  state->player.pos = make_vector2(back_buffer->width/2.0f, back_buffer->height/2.0f);
  state->player.dpos = make_vector2(0.0f, 0.0f);

  state->sprite_index = 48;
  state->sprite_count_x = 1;
  state->sprite_count_y = 2;

  state->enemy.is_alive = true;
  state->enemy.speed = 32.0f;
  state->enemy.size = make_vector2(state->tile_size, state->tile_size);
  state->enemy.pos.x = (f32)(rand() % back_buffer->width);
  state->enemy.pos.y = -state->tile_size;
  state->enemy.dpos = make_vector2(0.0f, 1.0f);
  state->enemy_max_hp = 3;
  state->enemy_hp = state->enemy_max_hp;

  state->next_bullet = 0;
  state->max_bullet_count = 32;
  state->bullets = push_array(&state->perm_arena, Entity, state->max_bullet_count);
  mem_zero_array(state->bullets, state->max_bullet_count);

  state->max_star_count = 32;
  state->stars = push_array(&state->perm_arena, Entity, state->max_star_count);
  mem_zero_array(state->stars, state->max_star_count);

  Date_Time date_time = platform_get_date_time();
  Dense_Time dense_time = dense_time_from_date_time(date_time);

  srand((u32)dense_time);
  for (u32 i = 0; i < state->max_star_count; ++i) {
    Entity *star = state->stars + i;
    star->is_alive = true;
    star->radius = (f32)(rand() % 3);
    star->speed = (f32)(20 + rand() % (50 - 20));
    star->pos.x = (f32)(rand() % back_buffer->width);
    star->pos.y = (f32)(rand() % back_buffer->height);
    star->dpos = make_vector2(0.0f, -1.0f);
  }

  state->draw_debug_info = true;
}

internal b32
entity_collide(Entity a, Entity b) {
  Vector2 half_size = vector2_div(a.size, 2.0f);
  f32 a_left = a.pos.x - half_size.x;
  f32 a_top = a.pos.y - half_size.y;
  f32 a_right = a.pos.x + half_size.x;
  f32 a_bottom = a.pos.y + half_size.y;

  half_size = vector2_div(b.size, 2.0f);
  f32 b_left = b.pos.x - half_size.x;
  f32 b_top = b.pos.y - half_size.y;
  f32 b_right = b.pos.x + half_size.x;
  f32 b_bottom = b.pos.y + half_size.y;

  b32 result = true;
  if ((a_top > b_bottom) ||
      (b_top > a_bottom) ||
      (a_left > b_right) ||
      (b_left > a_right)) {
    result = false;
  }
  return(result);
}

shared_function
KRUEGER_FRAME_PROC(krueger_frame) {
  Game_State *state = (Game_State *)memory->memory_ptr;
  Digital_Button *kbd = input->kbd;
  Image draw_buffer = *back_buffer;
  // image_clear(draw_buffer, 0xFF00FF);
  // s32 dw = (s32)(draw_buffer.width*0.6f);
  // draw_buffer = make_subimage(draw_buffer, 0, 0, dw, draw_buffer.height);
  
  /////////////////////
  // NOTE: begin update

  // NOTE: update player
  if (state->player.is_alive) {
    state->player.sprite_index = 1;
    if (kbd[KEY_UP].is_down) {
      state->player.dpos.y = -1.0f;
    }
    if (kbd[KEY_LEFT].is_down) {
      state->player.dpos.x = -1.0f;
      state->player.sprite_index = 0;
    }
    if (kbd[KEY_DOWN].is_down) {
      state->player.dpos.y = 1.0f;
    }
    if (kbd[KEY_RIGHT].is_down) {
      state->player.dpos.x = 1.0f;
      state->player.sprite_index = 2;
    }

    Vector2 player_dpos = vector2_mul(state->player.dpos, state->player.speed*time->dt_sec);
    state->player.pos = vector2_add(state->player.pos, player_dpos);
    state->player.dpos = make_vector2(0.0f, 0.0f);

    if (state->player.pos.x < 0.0f) {
      state->player.pos.x = 0;
    }
    if (state->player.pos.y < 0.0f) {
      state->player.pos.y = 0;
    }
    if (state->player.pos.x > (f32)draw_buffer.width) {
      state->player.pos.x = (f32)draw_buffer.width;
    }
    if (state->player.pos.y > (f32)draw_buffer.height) {
      state->player.pos.y = (f32)draw_buffer.height;
    }

    // NOTE: add bullets
    if (state->shoot_t <= 0.0f) {
      if (kbd[KEY_Z].is_down) {
        state->shoot_t = state->shoot_cd;
        state->r = state->max_r;
        Entity *bullet = state->bullets + state->next_bullet++;
        if (state->next_bullet >= state->max_bullet_count) {
          state->next_bullet = 0;
        }
        bullet->is_alive = true;
        bullet->sprite_index = 3;
        bullet->size = make_vector2(state->tile_size, state->tile_size);
        bullet->pos = vector2_sub(state->player.pos, make_vector2(0.0f, 2.0f));
        bullet->dpos = make_vector2(0.0f, -1.0f);
        bullet->speed = 320.0f;
      }
    } else {
      state->shoot_t -= 1.0f*time->dt_sec;
    }
  }

  // NOTE: update bullets
  for (u32 i = 0; i < state->max_bullet_count; ++i) {
    Entity *bullet = state->bullets + i;
    if (bullet->is_alive) {
      Vector2 dpos = vector2_mul(bullet->dpos, bullet->speed*time->dt_sec);
      bullet->pos = vector2_add(bullet->pos, dpos);
      if (bullet->pos.y <= -state->tile_size) {
        bullet->is_alive = false;
      }
    }
  }

  // NOTE: update enemy
  if (state->enemy.is_alive) {
    Vector2 enemy_dpos = vector2_mul(state->enemy.dpos, state->enemy.speed*time->dt_sec);
    state->enemy.pos = vector2_add(state->enemy.pos, enemy_dpos);
    if(state->enemy.pos.y > draw_buffer.height + state->tile_size*2.0f) {
      state->enemy.pos.x = (f32)(rand() % back_buffer->width);
      state->enemy.pos.y = -(state->tile_size*2.0f);
    }
    if (state->flash_t > 0.0f) {
      state->flash_t -= 1.0f*time->dt_sec;
    }
  }

  // NOTE: player x enemy collision
  if (state->invul_t <= 0.0f) {
    if (state->enemy.is_alive && state->player.is_alive) {
      if (entity_collide(state->enemy, state->player)) {
        state->invul_t = state->invul_cd;
        state->hp -= 1;
        if (state->hp <= 0) {
          state->player.is_alive = false;
        }
        state->enemy_hp = 0;
        if (state->enemy_hp <= 0) {
          state->enemy.pos.x = (f32)(rand() % back_buffer->width);
          state->enemy.pos.y = -state->tile_size;
          state->enemy_hp = state->enemy_max_hp;
          // state->enemy.is_alive = false;
        }
      }
    }
  } else {
    state->invul_t -= 1.0f*time->dt_sec;
  }

  // NOTE: enemy x bullets collision
  for (u32 i = 0; i < state->max_bullet_count; ++i) {
    Entity *bullet = state->bullets + i;
    if (bullet->is_alive && state->enemy.is_alive) {
      if (entity_collide(state->enemy, *bullet)) {
        bullet->is_alive = false;
        state->flash_t = state->flash_cd;
        state->enemy_hp -= 1;
        if (state->enemy_hp <= 0) {
          // state->enemy.is_alive = false;
          state->enemy.pos.x = (f32)(rand() % back_buffer->width);
          state->enemy.pos.y = -state->tile_size;
          state->enemy_hp = state->enemy_max_hp;
          state->score += 100;
        }
      }
    }
  }

  // NOTE: update stars
  for (u32 i = 0; i < state->max_star_count; ++i) {
    Entity *star = state->stars + i;
    star->pos.y += star->speed*time->dt_sec;
    if (star->pos.y - state->tile_size > (f32)draw_buffer.height) {
      star->pos.x = (f32)(rand() % back_buffer->width);
      star->pos.y = -star->radius;
    }
  }
  
  state->sec += time->dt_sec;

  ///////////////////
  // NOTE: begin draw

  image_clear(draw_buffer, CP_BLACK);

  // NOTE: draw stars
  for (u32 i = 0; i < state->max_star_count; ++i) {
    Entity *star = state->stars + i;
    u32 color = CP_DARK_BLUE;
    if (star->speed >= 35.0f) {
      color = CP_DARK_GRAY;
    }
    if (star->radius <= 1.0f) {
      draw_circle_line(draw_buffer,
                       (s32)star->pos.x, (s32)star->pos.y,
                       (s32)star->radius, color);
    } else {
      draw_circle_fill_vec(draw_buffer, star->pos, star->radius, color);
    }
  }

  // NOTE: draw enemy
  if (state->enemy.is_alive) {
    Image sprite = get_spritex(state->sprite_sheet, state->sprite_index,
                               state->sprite_count_x, state->sprite_count_y);
    pixel_shader_proc *shader = 0;
    if (state->flash_t > 0.0f) shader = shader_turn_all_white;
    Vector2 pos = align_position(sprite, state->enemy.pos, vec2(0.5f, 0.5f));
    draw_sprite_shader(draw_buffer, sprite, pos, shader, 0);
  }

  // NOTE: draw bullets
  for (u32 i = 0; i < state->max_bullet_count; ++i) { 
    Entity *bullet = state->bullets + i;
    if (bullet->is_alive) {
      Image sprite = get_sprite(state->sprite_sheet, bullet->sprite_index);
      Vector2 pos = align_position(sprite, bullet->pos, vec2(0.5f, 0.5f));
      draw_sprite_vec(draw_buffer, sprite, pos);
    }
  }

  { // NOTE: draw shooting EFX
    if (state->r > 0.0f) state->r -= 60.0f*time->dt_sec;
    if (state->r < 0.0f) state->r = 0.0f;
    Vector2 pos = state->player.pos;
    pos.y -= 4.5f;
    draw_circle_fill_vec(draw_buffer, pos, state->r, CP_WHITE);
  }

  // NOTE: draw player
  if (state->player.is_alive) {
    if (state->invul_t <= 0.0f) {
      { // NOTE: draw ship flame
        local f32 ms = 0;
        ms += time->dt_sec*thousand(1.0f);
        if (ms >= 32.0f) {
          state->flame_sprite_index += 1;
          if (state->flame_sprite_index > 20) {
            state->flame_sprite_index = 16;
          }
          ms = 0.0f;
        }
        Image sprite = get_sprite(state->sprite_sheet, state->flame_sprite_index);
        Vector2 pos = align_position(sprite, state->player.pos, vec2(0.5f, 0.5f));
        pos.y += 8.0f;
        draw_sprite_vec(draw_buffer, sprite, pos);
      }

      { // NOTE: draw ship
        Image sprite = get_sprite(state->sprite_sheet, state->player.sprite_index);
        Vector2 pos = align_position(sprite, state->player.pos, vec2(0.5f, 0.5f));
        draw_sprite_vec(draw_buffer, sprite, pos);
      }
    } else { // NOTE: invul state
      if (sin_f32(thousand(1.0f)*state->sec) <= 0.0f) { // NOTE: draw ship
        Image sprite = get_sprite(state->sprite_sheet, state->player.sprite_index);
        Vector2 pos = align_position(sprite, state->player.pos, vec2(0.5f, 0.5f));
        draw_sprite_vec(draw_buffer, sprite, pos);
      }
    }
  }

  ////////////////
  // NOTE: draw UI

  { // NOTE: player HP
    for (u32 i = 0; i < state->hp; ++i) {
      Image sprite = get_sprite(state->sprite_sheet, state->hp_sprite_index);
      Vector2 pos = make_vector2(0.0f, (f32)(draw_buffer.height - sprite.height));
      pos.x += i*sprite.width;
      draw_sprite_vec(draw_buffer, sprite, pos);
    }
  }

  Temp temp = temp_begin(&state->temp_arena);
  { // NOTE: score
    String8 score_str = push_str8_fmt(temp.arena, "%d", state->score);
    s32 x = draw_buffer.width;
    s32 y = draw_buffer.height;
    draw_text_align(draw_buffer, state->font, score_str,
                    x, y, 1.0f, 1.0f, CP_WHITE);
  }

#if 1
  {
    s32 x = draw_buffer.width/2;
    s32 y = draw_buffer.height/3;
    draw_text_align(draw_buffer, state->font, str8_lit("STAR FIGHTER"),
                    x, y, 0.5f, 0.5f, CP_WHITE);

    y = draw_buffer.height - y;
    if (sin_f32(10.0f*state->sec) < 0.5f) {
      draw_text_align(draw_buffer, state->font, str8_lit("PRESS [Z] TO START"),
                      x, y, 0.5f, 0.5f, CP_WHITE);
    }
  }
#endif

  // NOTE: draw debug info
  if (kbd[KEY_F1].pressed) state->draw_debug_info = !state->draw_debug_info;
  if (state->draw_debug_info) {
    f32 fps = million(1.0f)/time->_dt_us;
    f32 ms = time->_dt_us/thousand(1.0f);

    String8 fps_str = push_str8_fmt(temp.arena, "%.2f FPS", fps);
    String8 ms_str = push_str8_fmt(temp.arena, "%.2f MS", ms);
    
    s32 x = draw_buffer.width;
    s32 y = 0;
    draw_text_align(draw_buffer, state->font, fps_str,
                    x, y, 1.0f, 0.0f, CP_WHITE);

    x -= state->font.sheet.tile_w;
    y += state->font.sheet.tile_h;
    draw_text_align(draw_buffer, state->font, ms_str,
                    x, y, 1.0f, 0.0f, CP_WHITE);
  }
  temp_end(temp);
}
