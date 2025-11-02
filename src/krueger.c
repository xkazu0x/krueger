#include "krueger_base.h"
#include "krueger_platform.h"
#include "krueger_shared.h"

#include "krueger_base.c"
#include "krueger_platform.c"

#include <time.h>

// TODO:
// - Correct Player Diagonal Movement
// - Game Start and Game Over Screen

//////////////////////
// NOTE: Helper Macros

#define mask_alpha(col) (((col) >> 24) & 0xFF)
#define mask_red(col)   (((col) >> 16) & 0xFF)
#define mask_green(col) (((col) >>  8) & 0xFF)
#define mask_blue(col)  (((col) >>  0) & 0xFF)

#define pack_rgba(r, g, b, a) ((a << 24) | (r << 16) | (g << 8) | (b << 0))
#define pack_rgb(r, g, b) ((r << 16) | (g << 8) | (b << 0))

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
  f32 size;
  f32 speed;
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

  u32 tile_size;
  u32 color_table[16];
  u32 text_col;
  
  u32 score;
  u32 hp;
  u32 hp_spr;
  u32 flame_spr;
  f32 max_r;
  f32 r;
  f32 shoot_threshold_ms;
  f32 shoot_current_ms;
  Entity player;
  
  u32 curr_bullet;
  u32 max_bullet_count;
  Entity *bullets;

  u32 max_star_count;
  Entity *stars;
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
draw_rect_fillf(Image dst,
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
draw_rect_fillv(Image dst, Vector2 min, Vector2 max, u32 color) {
  draw_rect_fillf(dst, min.x, min.y, max.x, max.y, color);
}

internal void
draw_circle(Image dst,
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
draw_circle_fillf(Image dst,
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
draw_circle_fillv(Image dst, Vector2 pos, f32 r, u32 color) {
  draw_circle_fillf(dst, pos.x, pos.y, r, color);
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
draw_spritec(Image dst, Image src, s32 x, s32 y, u32 col) {
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
        f32 rt = mask_red(col)/255.0f;
        f32 gt = mask_green(col)/255.0f;
        f32 bt = mask_blue(col)/255.0f;
        u8 r = round_t(u8, rt*mask_red(src_color));
        u8 g = round_t(u8, gt*mask_green(src_color));
        u8 b = round_t(u8, bt*mask_blue(src_color));
        u32 color = pack_rgb(r, g, b);
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
draw_spritef(Image dst, Image src, f32 x, f32 y) {
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
draw_spritev(Image dst, Image src, Vector2 pos) {
  draw_spritef(dst, src, pos.x, pos.y);
}

internal void
draw_sprite_center(Image dst, Image src, Vector2 pos) {
  Vector2 half_size = make_vector2(src.width/2.0f, src.height/2.0f);
  pos = vector2_sub(pos, half_size);
  draw_spritev(dst, src, pos);
}

internal void
draw_text(Image dst, Font font, char *text,
          s32 x, s32 y, u32 col) {
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
    draw_spritec(dst, sprite, glyph_x, glyph_y, col);
    glyph_x += glyph_w;
  }
}

internal void
draw_text_left(Image dst, Font font, char *text,
               s32 x, s32 y, u32 col) {
  draw_text(dst, font, text, x, y, col);
}

internal void
draw_text_right(Image dst, Font font, char *text,
                s32 x, s32 y, u32 col) {
  s32 len = (s32)cstr_len(text);
  x = x - len*font.sheet.tile_w;
  draw_text(dst, font, text, x, y, col);
}

internal void
draw_text_center(Image dst, Font font, char *text,
                 s32 x, s32 y, u32 col) {
  s32 len = (s32)cstr_len(text);
  x = x - len*font.sheet.tile_w/2;
  draw_text(dst, font, text, x, y, col);
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

  state->tile_size = 8;
  state->color_table[0] = 0x000000;
  state->color_table[1] = 0x1D2B53;
  state->color_table[2] = 0x7E2553;
  state->color_table[3] = 0x008751;
  state->color_table[4] = 0xAB5236;
  state->color_table[5] = 0x5F574F;
  state->color_table[6] = 0xC2C3C7;
  state->color_table[7] = 0xFFF1E8;
  state->color_table[8] = 0xFF004D;
  state->color_table[9] = 0xFFA300;
  state->color_table[10] = 0xFFEC27;
  state->color_table[11] = 0x00E436;
  state->color_table[12] = 0x29ADFF;
  state->color_table[13] = 0x83769C;
  state->color_table[14] = 0xFF77A8;
  state->color_table[15] = 0xFFCCAA;
  state->text_col = 7;

  state->score = 1000;
  state->hp = 3;
  state->hp_spr = 32;
  state->flame_spr = 16;
  state->max_r = 7.0f;
  state->shoot_threshold_ms = 100.0f;
  state->shoot_current_ms = state->shoot_threshold_ms;

  state->player.sprite_index = 1;
  state->player.speed = 100.0f;
  state->player.pos = make_vector2(back_buffer->width/2.0f, back_buffer->height/2.0f);
  
  state->curr_bullet = 0;
  state->max_bullet_count = 32;
  state->bullets = arena_push_array(&state->perm_arena, Entity, state->max_bullet_count);
  mem_zero_array(state->bullets, state->max_bullet_count);

  state->max_star_count = 32;
  state->stars = arena_push_array(&state->perm_arena, Entity, state->max_star_count);
  mem_zero_array(state->stars, state->max_star_count);

  Date_Time date_time = platform_get_date_time();
  Dense_Time dense_time = dense_time_from_date_time(date_time);
  srand((u32)dense_time);
  for (u32 i = 0; i < state->max_star_count; ++i) {
    Entity *star = state->stars + i;
    star->is_alive = true;
    star->size = (f32)(rand() % 4);
    star->speed = (f32)(20 + rand() % (80 - 20));
    star->pos.x = (f32)(rand() % back_buffer->width);
    star->pos.y = (f32)(rand() % back_buffer->height);
    star->dpos = make_vector2(0.0f, -1.0f);
  }
}

shared_function
KRUEGER_FRAME_PROC(krueger_frame) {
  Game_State *state = (Game_State *)memory->memory_ptr;
  Digital_Button *kbd = input->kbd;
  Image draw_buffer = *back_buffer;

  // NOTE: Update Player
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

  Vector2 player_dpos = vector2_mul(state->player.dpos, state->player.speed*time->dt);
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

  // NOTE: Update Bullets
  state->shoot_current_ms += time->dt_ms;
  if (state->shoot_current_ms > state->shoot_threshold_ms) {
    state->shoot_current_ms = state->shoot_threshold_ms;
  }

  if (kbd[KEY_Z].is_down) {
    if (state->shoot_current_ms == state->shoot_threshold_ms) {
      Entity *bullet = state->bullets + state->curr_bullet++;
      bullet->is_alive = true;
      bullet->sprite_index = 3;
      bullet->pos = vector2_sub(state->player.pos, make_vector2(0.0f, 2.0f));
      bullet->dpos = make_vector2(0.0f, -1.0f);
      bullet->speed = 320.0f;
      if (state->curr_bullet >= state->max_bullet_count) {
        state->curr_bullet = 0;
      }
      state->r = state->max_r;
      state->shoot_current_ms = 0.0f;
    }
  }
  
  for (u32 i = 0; i < state->max_bullet_count; ++i) {
    Entity *bullet = state->bullets + i;
    if (bullet->is_alive) {
      Vector2 dpos = vector2_mul(bullet->dpos, bullet->speed*time->dt);
      bullet->pos = vector2_add(bullet->pos, dpos);
      if (bullet->pos.y <= -(f32)state->tile_size) {
        bullet->is_alive = false;
      }
    }
  }

  // NOTE: Update Stars
  for (u32 i = 0; i < state->max_star_count; ++i) {
    Entity *star = state->stars + i;
    star->pos.y += star->speed*time->dt;
    if (star->pos.y - (f32)state->tile_size > (f32)draw_buffer.height) {
      star->pos.x = (f32)(rand() % back_buffer->width);
      star->pos.y = -star->size;
    }
  }

  //////////////////////
  // NOTE: Draw Rountine
  clear(draw_buffer, 0x000000);

  // NOTE: Draw Stars
  for (u32 i = 0; i < state->max_star_count; ++i) {
    Entity *star = state->stars + i;
    u32 color = 0;
    if (star->speed < 40.0f) {
      color = state->color_table[1];
    } else if (star->speed >= 40.0f && star->speed < 60.0f) {
      color = state->color_table[13];
    } else {
      color = state->color_table[6];
    }
    if (star->size <= 1.0f) {
      draw_circle(draw_buffer, (s32)star->pos.x, (s32)star->pos.y, (s32)star->size, color);
    } else {
      draw_circle_fillv(draw_buffer, star->pos, star->size, color);
    }
  }

  // NOTE: Draw Bullets
  for (u32 i = 0; i < state->max_bullet_count; ++i) { 
    Entity *bullet = state->bullets + i;
    if (bullet->is_alive) {
      Image sprite = get_sprite(state->sprite_sheet, bullet->sprite_index);
      draw_sprite_center(draw_buffer, sprite, bullet->pos);
    }
  }

  {
    // NOTE: Draw Shooting EFX
    if (state->r > 0.0f) state->r -= 60.0f*time->dt;
    if (state->r < 0.0f) state->r = 0.0f;
    Vector2 pos = state->player.pos;
    pos.y -= 4.5f;
    draw_circle_fillv(draw_buffer, pos, state->r, state->color_table[7]);
  }

  {
    // NOTE: Draw Ship Flame
    local f32 ms = 0;
    ms += time->dt*1000.0f;
    if (ms >= 32.0f) {
      state->flame_spr += 1;
      if (state->flame_spr > 20) {
        state->flame_spr = 16;
      }
      ms = 0.0f;
    }
    Image sprite = get_sprite(state->sprite_sheet, state->flame_spr);
    Vector2 pos = state->player.pos;
    pos.y += 8.0f;
    draw_sprite_center(draw_buffer, sprite, pos);
  }

  {
    // NOTE: Draw Ship
    Image sprite = get_sprite(state->sprite_sheet, state->player.sprite_index);
    draw_sprite_center(draw_buffer, sprite, state->player.pos);
  }

  ////////////////
  // NOTE: Draw UI
  {
    // NOTE: Player HP
    Image sprite = get_sprite(state->sprite_sheet, state->hp_spr);
    Vector2 pos = make_vector2(0.0f, (f32)(draw_buffer.height - sprite.height));
    draw_spritev(draw_buffer, sprite, pos);
  }

  Temp temp = temp_begin(&state->temp_arena);
  {
    s32 size = sprintf((char*)(temp.arena->buf + temp.arena->cmt_size), "%dx", state->hp);
    char *hp_str = arena_push_array(temp.arena, char, size);
    s32 x = state->tile_size;
    s32 y = draw_buffer.height - state->font.sheet.tile_h - 1;
    draw_text_left(draw_buffer, state->font, hp_str, x, y,
                   state->color_table[state->text_col]);
  }

#if 0
  {
    s32 x = draw_buffer.width/2;
    s32 y = draw_buffer.height/3;
    draw_text_center(draw_buffer, state->font, "KRUEGER", x, y,
                     state->color_table[state->text_col]);
    y = draw_buffer.height - y;
    local b32 draw = true;
    local f32 ms = 0;
    ms += time->dt_ms;
    if (ms >= 300.0f) {
      draw = !draw;
      ms = 0.0f;
    }
    if (draw) {
      draw_text_center(draw_buffer, state->font, "PRESS [Z] TO START", x, y,
                       state->color_table[state->text_col]);
    }
  }
#endif

  {
    s32 size = sprintf((char*)(temp.arena->buf + temp.arena->cmt_size), "SCORE:%d", state->score);
    char *score_str = arena_push_array(temp.arena, char, size);
    draw_text(draw_buffer, state->font, score_str, 0, 0,
              state->color_table[state->text_col]);
  }

  {
    // NOTE: Draw Debug Info
    f32 ms = time->dt_us/thousand(1.0f);
    s32 size = sprintf((char*)(temp.arena->buf + temp.arena->cmt_size), "%.2fms", ms);
    char *ms_str = arena_push_array(temp.arena, char, size);
    s32 x = draw_buffer.width;
    // s32 y = draw_buffer.height - state->font.sheet.tile_h;
    s32 y = 0;
    draw_text_right(draw_buffer, state->font, ms_str, x, y,
                    state->color_table[state->text_col]);
  }
  temp_end(temp);
}
