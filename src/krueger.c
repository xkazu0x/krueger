#include "krueger_base.h"
#include "krueger_platform.h"
#include "krueger_random.h"

#include "krueger_base.c"
#include "krueger_platform.c"
#include "krueger_random.c"

#include "krueger_shared.h"

// TODO:
// - ememy waves
// - correct player diagonal movement

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

internal Image
load_bmp(String8 filepath, Arena *perm_arena, Arena *temp_arena) {
  Image result = {0};
  Temp temp = temp_begin(temp_arena);
  void *file_data = platform_read_entire_file(temp.arena, filepath);
  if (file_data) {
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
  u32 max_hp;
  u32 hp;

  f32 flash_t;
  f32 invul_t;
  f32 shoot_t;
  f32 flash_cd;
  f32 invul_cd;
  f32 shoot_cd;

  f32 speed;
  Vector2 size;
  Vector2 pos;
  Vector2 dpos;

  u32 sprite_index;
  u32 sprite_count_x;
  u32 sprite_count_y;
} Entity;

typedef enum {
  PARTICLE_STAR,
  PARTICLE_SHIP_FLAME,
  PARTICLE_EXPLOSION,
} Particle_Type;

typedef struct {
  Particle_Type type;
  u32 color;
  f32 life;
  f32 dlife;
  f32 radius;
  f32 dradius;
  f32 max_speed;
  f32 speed;
  f32 dspeed;
  Vector2 pos;
  Vector2 dpos;
  f32 age;
  f32 max_age;
  f32 fric;
} Particle;

typedef enum {
  GAME_SCREEN_MENU,
  GAME_SCREEN_PLAY,
  GAME_SCREEN_OVER,
} Game_Screen_Type;

typedef struct {
  Random_Series entropy;
  Arena perm_arena;
  Arena temp_arena;
  Image font_image;
  Image sprites_image;
  Sprite_Sheet font_sheet;
  Sprite_Sheet sprite_sheet;

  Font font;
  f32 tile_size;
  Game_Screen_Type screen_type;

  u32 score;
  u32 wave_index;
  f32 sec;

  f32 player_max_shoot_r;
  f32 player_shoot_r;
  f32 player_dshoot_r;
  
  u32 flame_sprite_index;
  u32 player_hp_sprite_index;
  u32 player_shoot_side;
  Entity player;

  u32 next_bullet;
  u32 bullet_count;
  Entity *bullets;

  u32 enemy_alive_count;
  u32 enemy_count;
  Entity *enemies;

  u32 next_ship_flame_particle;
  u32 next_explosion_particle;

  u32 star_particle_count;
  u32 ship_flame_particle_count;
  u32 explosion_particle_count;
  u32 particle_count;

  Particle *particles;
  Particle *star_particles;
  Particle *ship_flame_particles;
  Particle *explosion_particles;

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
                 s32 cx, s32 cy, f32 r,
                 u32 color) {
  r = abs_t(f32, r);
  s32 min_x = clamp_bot(0, cx - round_t(s32, r));
  s32 min_y = clamp_bot(0, cy - round_t(s32, r));
  s32 max_x = clamp_top(cx + round_t(s32, r), (s32)dst.width);
  s32 max_y = clamp_top(cy + round_t(s32, r), (s32)dst.height);
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

PIXEL_SHADER_PROC(shader_replace_color) {
  u32 result = *(u32 *)user_data;
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

internal void
game_start(Game_State *state, Image draw_buffer) {
  state->score = 0;
  state->wave_index = 1;
  state->sec = 0.0f;

  state->player.is_alive = true;
  state->player.max_hp = 3;
  state->player.hp = state->player.max_hp;

  state->player.shoot_t = 0.0f;
  state->player.invul_t = 0.0f;
  state->player.shoot_cd = 0.1f;
  state->player.invul_cd = 1.2f;

  state->player.speed = 100.0f;
  state->player.size = make_vector2(state->tile_size, state->tile_size);
  state->player.pos = make_vector2(draw_buffer.width/2.0f,
                                   draw_buffer.height - draw_buffer.height/4.0f);
  state->player.dpos = make_vector2(0.0f, 0.0f);
  state->player.sprite_index = 1;

  for (u32 bullet_index = 0;
       bullet_index < state->bullet_count;
       ++bullet_index) {
    Entity *bullet = state->bullets + bullet_index;
    bullet->is_alive = false;
  }
  
  state->enemy_alive_count = 0;
  for (u32 enemy_index = 0;
       enemy_index < state->enemy_count;
       ++enemy_index) {
    Entity *enemy = state->enemies + enemy_index;
    enemy->is_alive = false;
  }
}

shared_function
KRUEGER_INIT_PROC(krueger_init) {
  assert(sizeof(Game_State) <= memory->memory_size);
  Game_State *state = (Game_State *)memory->memory_ptr;

  Date_Time date_time = platform_get_date_time();
  Dense_Time dense_time = dense_time_from_date_time(date_time);
  state->entropy = random_seed((u32)dense_time);

  uxx half_memory_size = memory->memory_size/2;
  uxx perm_memory_size = half_memory_size - sizeof(Game_State);
  uxx temp_memory_size = half_memory_size;

  u8 *perm_memory_ptr = (u8 *)memory->memory_ptr + sizeof(Game_State);
  u8 *temp_memory_ptr = memory->memory_ptr + half_memory_size;

  state->perm_arena = make_arena(perm_memory_ptr, perm_memory_size);
  state->temp_arena = make_arena(temp_memory_ptr, temp_memory_size);

  state->font_image     = load_bmp(str8_lit("../res/pico8_font.bmp"),
                                   &state->perm_arena, &state->temp_arena);
  state->sprites_image  = load_bmp(str8_lit("../res/sprites.bmp"),
                                   &state->perm_arena, &state->temp_arena);

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
  state->screen_type = GAME_SCREEN_MENU;
  
  state->player_max_shoot_r = 7.0f;
  state->player_shoot_r = 0.0f;
  state->player_dshoot_r = -80.0f;

  state->flame_sprite_index = 16;
  state->player_hp_sprite_index = 32;

  state->next_bullet = 0;
  state->bullet_count = 32;
  state->bullets = push_array(&state->perm_arena, Entity, state->bullet_count);
  mem_zero(state->bullets, state->bullet_count);

  state->enemy_count = 4;
  state->enemies = push_array(&state->perm_arena, Entity, state->enemy_count);
  mem_zero(state->enemies, state->enemy_count);

  state->star_particle_count = 32;
  state->ship_flame_particle_count = 16;
  state->explosion_particle_count = 256;
  state->particle_count = (state->star_particle_count + 
                           state->ship_flame_particle_count +
                           state->explosion_particle_count);
  state->particles = push_array(&state->perm_arena, Particle, state->particle_count);
  
  s32 offset = 0;
  state->star_particles = state->particles;

  offset += state->star_particle_count;
  state->ship_flame_particles = state->particles + offset;

  offset += state->ship_flame_particle_count;
  state->explosion_particles = state->particles + offset;

  for (u32 i = 0; i < state->star_particle_count; ++i) {
    Particle *particle = state->star_particles + i;
    particle->color = CP_DARK_BLUE;
    particle->radius = (f32)random_choice(&state->entropy, 3);
    particle->speed = random_range(&state->entropy, 20.0f, 50.0f);
    if (particle->speed >= 35.0f) particle->color = CP_DARK_GRAY;
    particle->pos.x = random_unilateral(&state->entropy)*back_buffer->width;
    particle->pos.y = random_unilateral(&state->entropy)*back_buffer->height;
    particle->dpos = make_vector2(0.0f, -1.0f);
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

// TODO: OLD, BUT COOL. THE ANIMATION NEEDS TO BE FASTER
internal void
_add_explosion_particles(Game_State *state) {
#if 0
  for (u32 j = 0; j < 4; ++j) {
    Particle *particle = state->explosion_particles + state->next_explosion_particle++;
    if (state->next_explosion_particle >= array_count(state->explosion_particles)) {
      state->next_explosion_particle = 0;
    }
    particle->speed = 60.0f;
    particle->pos = state->enemy.pos;
    particle->pos.y += random_range(&state->entropy, -4.0f, 2.0f);
    particle->dpos = make_vector2(random_range(&state->entropy, -0.3f, 0.3f),
                                  -0.5f);
    particle->color = CP_DARK_RED;
    particle->radius = random_range(&state->entropy, 8.0f, 12.0f);
    particle->dradius = -5.0f;
  }
  for (u32 j = 0; j < 2; ++j) {
    Particle *particle = state->explosion_particles + state->next_explosion_particle++;
    if (state->next_explosion_particle >= array_count(state->explosion_particles)) {
      state->next_explosion_particle = 0;
    }
    particle->speed = 40.0f;
    particle->pos = state->enemy.pos;
    particle->pos.y += random_range(&state->entropy, -6.0f, 2.0f);
    particle->dpos = make_vector2(random_range(&state->entropy, -0.3f, 0.3f),
                                  -0.5f);
    particle->color = CP_RED;
    particle->radius = random_range(&state->entropy, 8.0f, 10.0f);
    particle->dradius = -5.0f;
  }
  for (u32 j = 0; j < 3; ++j) {
    Particle *particle = state->explosion_particles + state->next_explosion_particle++;
    if (state->next_explosion_particle >= array_count(state->explosion_particles)) {
      state->next_explosion_particle = 0;
    }
    particle->speed = 40.0f;
    particle->pos = state->enemy.pos;
    particle->dpos = make_vector2(random_range(&state->entropy, -0.2f, 0.2f),
                                  -random_unilateral(&state->entropy));
    particle->color = CP_ORANGE;
    particle->radius = random_range(&state->entropy, 8.0f, 10.0f);
    particle->dradius = -10.0f;
  }
  for (u32 j = 0; j < 3; ++j) {
    Particle *particle = state->explosion_particles + state->next_explosion_particle++;
    if (state->next_explosion_particle >= array_count(state->explosion_particles)) {
      state->next_explosion_particle = 0;
    }
    particle->speed = 20.0f;
    particle->pos = state->enemy.pos;
    particle->dpos = make_vector2(random_range(&state->entropy, -0.2f, 0.2f),
                                  -random_unilateral(&state->entropy));
    particle->color = CP_YELLOW;
    particle->radius = random_range(&state->entropy, 4.0f, 6.0f);
    particle->dradius = -4.0f;
  }
#endif
#if 0
  u32 color_table[] = {
    CP_DARK_BROWN,
    CP_DARK_RED,
    CP_RED,
    CP_ORANGE,
    CP_YELLOW,
  };
  for (u32 j = 0; j < 16; ++j) {
    Particle *particle = state->explosion_particles + state->next_explosion_particle++;
    if (state->next_explosion_particle >= array_count(state->explosion_particles)) {
      state->next_explosion_particle = 0;
    }
    particle->speed = random_range(&state->entropy, 50.0f, 100.0f);
    particle->pos = state->enemy.pos;
    particle->dpos = make_vector2(random_bilateral(&state->entropy),
                                  random_bilateral(&state->entropy));
    u32 color_index = random_choice(&state->entropy, array_count(color_table));
    particle->color = color_table[color_index];
    particle->radius = random_range(&state->entropy, 4.0f, 8.0f);
    particle->dradius = random_range(&state->entropy, -8.0f, -4.0f);
  }
#endif
}

internal void
spawn_ship_flame_particles(Game_State *state, Clock *time) {
  local f32 sec = 0.0f;
  sec += time->dt_sec;
  if (sec >= 0.15f) {
    Particle *particle = state->ship_flame_particles + state->next_ship_flame_particle++;
    if (state->next_ship_flame_particle >= state->ship_flame_particle_count) {
      state->next_ship_flame_particle = 0;
    }
    particle->type = PARTICLE_SHIP_FLAME;
    particle->color = CP_BLUE;

    particle->life = 10.0f;
    particle->dlife = -30.0f;

    particle->radius = 2.5f;
    particle->dradius = -9.0f;

    particle->speed = 50.0f;
    particle->dspeed = -150.0f;

    particle->pos = state->player.pos;
    particle->pos.y += state->tile_size*0.7f;
    particle->dpos = make_vector2(random_range(&state->entropy, -0.2f, 0.2f), 1.0f);
    sec = 0.0f;
  }
}

internal void
spawn_explosion_particles(Game_State *state, Vector2 pos) {
  {
    Particle *particle = state->explosion_particles + state->next_explosion_particle++;
    if (state->next_explosion_particle >= state->explosion_particle_count) {
      state->next_explosion_particle = 0;
    }
    particle->type = PARTICLE_EXPLOSION;
    particle->color = CP_WHITE;
    particle->speed = 0.0f;
    particle->age = 0.0f;
    particle->max_age = 0.0f;
    particle->radius = 10.0f;
    particle->dradius = 12.0f;
    particle->pos = pos;
    particle->dpos = make_vector2(random_bilateral(&state->entropy),
                                  random_bilateral(&state->entropy));
  }

  for (u32 i = 0; i < 32; ++i) {
    Particle *particle = state->explosion_particles + state->next_explosion_particle++;
    if (state->next_explosion_particle >= state->explosion_particle_count) {
      state->next_explosion_particle = 0;
    }
    particle->type = PARTICLE_EXPLOSION;
    particle->color = CP_WHITE;
    particle->age = random_range(&state->entropy, 0.0f, 0.5f);
    // particle->max_age = random_range(&state->entropy, 0.7f, 1.0f);
    particle->max_age = 1.0f;
    particle->radius = random_range(&state->entropy, 1.0f, 4.0f);
    particle->dradius = 10.0f;
    particle->fric = 420.0f;
    particle->speed = random_range(&state->entropy, 100.0f, 125.0f);
    particle->pos = pos;
    particle->dpos = make_vector2(random_bilateral(&state->entropy),
                                  random_bilateral(&state->entropy));
  }
}

shared_function
KRUEGER_FRAME_PROC(krueger_frame) {
  Game_State *state = (Game_State *)memory->memory_ptr;
  Digital_Button *kbd = input->kbd;
  state->sec += time->dt_sec;

  Image draw_buffer = *back_buffer; 
  image_fill(draw_buffer, CP_BLACK);

  // NOTE: update and draw particles
  for (u32 i = 0; i < state->particle_count; ++i) {
    Particle *particle = state->particles + i;
    switch (particle->type) {
      case PARTICLE_STAR: {
        particle->pos.y += particle->speed*time->dt_sec;
        if (particle->pos.y - particle->radius > (f32)draw_buffer.height) {
          particle->pos.x = random_unilateral(&state->entropy)*draw_buffer.width;
          particle->pos.y = -particle->radius;
        }

        if (particle->radius <= 1.0f) {
          draw_circle_line(draw_buffer,
                           (s32)particle->pos.x, (s32)particle->pos.y,
                           (s32)particle->radius, particle->color);
        } else {
          draw_circle_fill(draw_buffer,
                           (s32)particle->pos.x, (s32)particle->pos.y,
                           particle->radius, particle->color);
        }
      } break;
      case PARTICLE_SHIP_FLAME: {
        if (particle->life > 0.0f) {
          particle->life += particle->dlife*time->dt_sec;

          particle->radius += particle->dradius*time->dt_sec;
          if (particle->radius <= 0.0f) particle->radius = 0.0f;

          particle->speed += particle->dspeed*time->dt_sec;
          if (particle->speed <= 0.0f) particle->speed = 0.0f;

          Vector2 dpos = vector2_mul(particle->dpos, particle->speed*time->dt_sec);
          particle->pos = vector2_add(particle->pos, dpos);

          if (state->player.invul_t <= 0.0f) {
            draw_circle_fill(draw_buffer,
                             (s32)particle->pos.x, (s32)particle->pos.y,
                             particle->radius, particle->color);
          }
        }
      } break;
      case PARTICLE_EXPLOSION: {
        if (particle->radius > 0.0f) {
          Vector2 dpos = vector2_mul(particle->dpos, particle->speed*time->dt_sec);
          particle->pos = vector2_add(particle->pos, dpos);

          particle->speed -= particle->fric*time->dt_sec;
          if (particle->speed < 0.0f) particle->speed = 0.0f;

          particle->fric += (particle->fric*0.9f)*time->dt_sec;

          particle->age += 1.5f*time->dt_sec;
          if (particle->age > 0.15f) particle->color = CP_YELLOW;
          if (particle->age > 0.27f) particle->color = CP_ORANGE;
          if (particle->age > 0.43f) particle->color = CP_RED;
          if (particle->age > 0.7f) particle->color = CP_DARK_RED;
          if (particle->age > 0.9f) particle->color = CP_DARK_BROWN;
          if (particle->age > particle->max_age) {
            particle->radius -= particle->dradius*time->dt_sec;
          }

          draw_circle_fill(draw_buffer,
                           (s32)particle->pos.x, (s32)particle->pos.y,
                           particle->radius, particle->color);
        }
      } break;
    }
  }

  switch (state->screen_type) {
    case GAME_SCREEN_MENU: {
      if (kbd[KEY_Z].pressed) {
        state->screen_type = GAME_SCREEN_PLAY;
        game_start(state, draw_buffer);
      }

      { // NOTE: draw title
        s32 x = draw_buffer.width/2;
        s32 y = draw_buffer.height/3;
        draw_text_align(draw_buffer, state->font, str8_lit("STARFIGHTER"),
                        x, y, 0.5f, 0.5f, CP_RED);

        String8 text = str8_lit("press [z] to start");
        y = (s32)(draw_buffer.height - state->tile_size*4.0f);
        if (sin_f32(state->sec*22.0f) > -0.5f) {
          draw_text_align(draw_buffer, state->font, text,
                          x, y, 0.5f, 0.5f, CP_DARK_BROWN);
        } else {
          draw_text_align(draw_buffer, state->font, text,
                          x, y, 0.5f, 0.5f, CP_GRAY);
        }
      }
    } break;
    case GAME_SCREEN_PLAY: {
      // NOTE: update player
      if (state->player.is_alive) {
        spawn_ship_flame_particles(state, time);

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

        // NOTE: player spawn bullets
        if (state->player.shoot_t <= 0.0f) {
          if (kbd[KEY_Z].is_down) {
            state->player.shoot_t = state->player.shoot_cd;
            state->player_shoot_r = state->player_max_shoot_r;

            Entity *bullet = state->bullets + state->next_bullet++;
            if (state->next_bullet >= state->bullet_count) {
              state->next_bullet = 0;
            }

            bullet->is_alive = true;
            bullet->speed = 380.0f;
            bullet->size = make_vector2(state->tile_size, state->tile_size);
            bullet->pos = state->player.pos;
            f32 gap = 2.0f;
            bullet->pos.x += (state->player_shoot_side++%2 == 0) ? gap : -gap;
            bullet->dpos = make_vector2(0.0f, -1.0f);
            bullet->sprite_index = 4;
          }
        } else {
          state->player.shoot_t -= 1.0f*time->dt_sec;
        }
      }

      // NOTE: update and draw bullets
      for (u32 bullet_index = 0;
           bullet_index < state->bullet_count;
           ++bullet_index) {
        Entity *bullet = state->bullets + bullet_index;
        if (bullet->is_alive) {
          Vector2 dpos = vector2_mul(bullet->dpos, bullet->speed*time->dt_sec);
          bullet->pos = vector2_add(bullet->pos, dpos);
          if (bullet->pos.y <= -state->tile_size) {
            bullet->is_alive = false;
          }

          Image sprite = get_sprite(state->sprite_sheet, bullet->sprite_index);
          Vector2 pos = align_position(sprite, bullet->pos, vec2(0.5f, 0.5f));
          draw_sprite_vec(draw_buffer, sprite, pos);
        }
      }
      
      // NOTE: spawn enemies
      if (state->player.is_alive) {
        local f32 sec = 1.0f;
        sec += time->dt_sec;
        if (sec >= 1.0f) {
          if (state->enemy_alive_count < state->enemy_count) {
            for (u32 i = 0; i < state->enemy_count; ++i) {
              Entity *enemy = state->enemies + i;
              if (!enemy->is_alive) {
                enemy->is_alive = true;
                enemy->hp = 3;

                enemy->flash_t = 0.0f;
                enemy->flash_cd = 0.05f;

                enemy->speed = 32.0f;
                enemy->size = make_vector2(state->tile_size, state->tile_size);
                enemy->pos.x = random_unilateral(&state->entropy)*back_buffer->width;
                enemy->pos.y = -enemy->size.y;
                enemy->dpos = make_vector2(0.0f, 1.0f);

                enemy->sprite_index = 48;
                // enemy->sprite_count_x = 1;
                // enemy->sprite_count_y = 2;
                
                state->enemy_alive_count += 1;
                break;
              }
            }
          }
          sec = 0.0f;
        }
      }
      
      // NOTE: update and draw enemies
      for (u32 i = 0; i < state->enemy_count; ++i) {
        Entity *enemy = state->enemies + i;
        if (enemy->is_alive) {
          Vector2 dpos = vector2_mul(enemy->dpos, enemy->speed*time->dt_sec);
          enemy->pos = vector2_add(enemy->pos, dpos);

          if(enemy->pos.y > draw_buffer.height + state->tile_size*2.0f) {
            enemy->is_alive = false;
            state->enemy_alive_count -= 1;
          }

          if (enemy->flash_t > 0.0f) {
            enemy->flash_t -= 1.0f*time->dt_sec;
          }

          Image sprite = get_sprite(state->sprite_sheet, enemy->sprite_index);
          // Image sprite = get_spritex(state->sprite_sheet, enemy->sprite_index,
          //                            enemy->sprite_count_x, enemy->sprite_count_y);

          pixel_shader_proc *shader = 0;
          u32 user_data = 0;
          if (enemy->flash_t > 0.0f) {
            shader = shader_replace_color;
            user_data = CP_RED;
          }

          Vector2 pos = align_position(sprite, enemy->pos, vec2(0.5f, 0.5f));
          draw_sprite_shader(draw_buffer, sprite, pos, shader, &user_data);
        }
      }

      // NOTE: player x enemy collision
      if (state->player.is_alive && state->player.invul_t <= 0.0f) {
        for (u32 enemy_index = 0;
             enemy_index < state->enemy_count;
             ++enemy_index) {
          Entity *enemy = state->enemies + enemy_index;
          if (enemy->is_alive) {
            if (entity_collide(state->player, *enemy)) {
              spawn_explosion_particles(state, enemy->pos);

              state->player.invul_t = state->player.invul_cd;
              state->player.hp -= 1;

              if (state->player.hp <= 0) {
                state->player.is_alive = false;
                state->sec = 0.0f;
              }

              enemy->hp = 0;
              enemy->is_alive = false;
              state->enemy_alive_count -= 1;
            }
          }
        }
      } else {
        state->player.invul_t -= 1.0f*time->dt_sec;
      }

      // NOTE: bullet x enemy collision
      for (u32 bullet_index = 0;
           bullet_index < state->bullet_count;
           ++bullet_index) {
        Entity *bullet = state->bullets + bullet_index;
        if (bullet->is_alive) {
          for (u32 enemy_index = 0;
               enemy_index < state->enemy_count;
               ++enemy_index) {
            Entity *enemy = state->enemies + enemy_index;
            if (enemy->is_alive) {
              if (entity_collide(*bullet, *enemy)) {
                bullet->is_alive = false;
                enemy->flash_t = enemy->flash_cd;
                enemy->hp -= 1;
                if (enemy->hp <= 0) {
                  spawn_explosion_particles(state, enemy->pos);
                  enemy->is_alive = false;
                  state->enemy_alive_count -= 1;
                  state->score += 100;
                }
              }
            }
          }
        }
      }

      ///////////////////
      // NOTE: begin draw

      { // NOTE: draw player shoot effect
        if (state->player_shoot_r > 0.0f) {
          state->player_shoot_r += state->player_dshoot_r*time->dt_sec;
        }
        if (state->player_shoot_r < 0.0f) state->player_shoot_r = 0.0f;
        Vector2 pos = state->player.pos;
        pos.y -= 4.0f;
        f32 gap = 2.0f;
        pos.x += (state->player_shoot_side%2 == 1) ? gap : -gap;
        draw_circle_fill(draw_buffer, (s32)pos.x, (s32)pos.y,
                         state->player_shoot_r, CP_WHITE);
      }

      // NOTE: draw player
      if (state->player.is_alive) {
        if (state->player.invul_t <= 0.0f) {
          { // NOTE: draw ship flame sprite
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
        } else {
          // NOTE: invul state
          if (sin_f32(thousand(1.0f)*state->sec) <= 0.0f) { // NOTE: draw ship
            Image sprite = get_sprite(state->sprite_sheet, state->player.sprite_index);
            Vector2 pos = align_position(sprite, state->player.pos, vec2(0.5f, 0.5f));
            draw_sprite_vec(draw_buffer, sprite, pos);
          }
        }
      }
      
      // NOTE: draw ui
      if (state->player.is_alive) {
        // NOTE: draw enemy alive count
        {
          Temp temp = temp_begin(&state->temp_arena);
          String8 str = push_str8_fmt(temp.arena, "%d/%d", state->enemy_alive_count, state->enemy_count);
          s32 x = 0;
          s32 y = 0;
          draw_text_align(draw_buffer, state->font, str, x, y, 0.0f, 0.0f, CP_WHITE);
          temp_end(temp);
        }

        // NOTE: draw player hp count
        for (u32 hp_index = 0;
             hp_index < state->player.hp;
             ++hp_index) {
          Image sprite = get_sprite(state->sprite_sheet, state->player_hp_sprite_index);
          Vector2 pos = make_vector2(0.0f, (f32)(draw_buffer.height - sprite.height));
          pos.x += hp_index*sprite.width + 1;
          pos.y -= 1;
          draw_sprite_vec(draw_buffer, sprite, pos);
        }

        { // NOTE: score
          Temp temp = temp_begin(&state->temp_arena);
          String8 str = push_str8_fmt(temp.arena, "%d", state->score);
          s32 x = draw_buffer.width - 1;
          s32 y = draw_buffer.height - 1;
          draw_text_align(draw_buffer, state->font, str, x, y, 1.0f, 1.0f, CP_WHITE);
          temp_end(temp);
        }

#if 0
        { // NOTE: draw wave index
          Temp temp = temp_begin(&state->temp_arena);
          String8 wave_str = push_str8_fmt(temp.arena, "WAVE %d", state->wave_index);

          s32 x = draw_buffer.width/2;
          s32 y = (s32)(state->tile_size*2.0f);

          if (sin_f32(state->sec*22.0f) > -0.5f) {
            draw_text_align(draw_buffer, state->font, wave_str,
                            x, y, 0.5f, 0.5f, CP_DARK_RED);
          } else {
            draw_text_align(draw_buffer, state->font, wave_str,
                            x, y, 0.5f, 0.5f, CP_RED);
          }
          temp_end(temp);
        }
#endif
      } else {
        if (kbd[KEY_Z].pressed) {
          state->screen_type = GAME_SCREEN_MENU;
        }

        { // NOTE: draw game over
          s32 x = draw_buffer.width/2;
          s32 y = draw_buffer.height/3;
          draw_text_align(draw_buffer, state->font, str8_lit("GAME OVER"),
                          x, y, 0.5f, 0.5f, CP_RED);

          String8 text_top = str8_lit("press [z] to go");
          String8 text_bot = str8_lit("back to menu");
          y = (s32)(draw_buffer.height - state->tile_size*4.0f - state->tile_size/2.0f);
          if (sin_f32(state->sec*22.0f) > -0.5f) {
            draw_text_align(draw_buffer, state->font, text_top,
                            x, y, 0.5f, 0.5f, CP_DARK_BROWN);
            y += state->font.sheet.tile_h;
            draw_text_align(draw_buffer, state->font, text_bot,
                            x, y, 0.5f, 0.5f, CP_DARK_BROWN);
          } else {
            draw_text_align(draw_buffer, state->font, text_top,
                            x, y, 0.5f, 0.5f, CP_GRAY);
            y += state->font.sheet.tile_h;
            draw_text_align(draw_buffer, state->font, text_bot,
                            x, y, 0.5f, 0.5f, CP_GRAY);
          }

          { // NOTE: score
            Temp temp = temp_begin(&state->temp_arena);
            String8 score_str = push_str8_fmt(temp.arena, "score:%d", state->score);
            y = draw_buffer.height/2;
            draw_text_align(draw_buffer, state->font, score_str,
                            x, y, 0.5f, 0.5f, CP_WHITE);
            temp_end(temp);
          }
        }
      }
    } break;
  }

  // NOTE: draw debug info
  Temp temp = temp_begin(&state->temp_arena);
  if (kbd[KEY_F1].pressed) state->draw_debug_info = !state->draw_debug_info;
  if (state->draw_debug_info) {
    f32 fps = million(1.0f)/time->_dt_us;
    f32 ms = time->_dt_us/thousand(1.0f);

    String8 fps_str = push_str8_fmt(temp.arena, "%.2f FPS", fps);
    String8 ms_str = push_str8_fmt(temp.arena, "%.2f MS", ms);
    
    s32 x = draw_buffer.width - 1;
    s32 y = 0;
    draw_text_align(draw_buffer, state->font, fps_str,
                    x, y, 1.0f, 0.0f, CP_GREEN);

    x -= state->font.sheet.tile_w;
    y += state->font.sheet.tile_h;
    draw_text_align(draw_buffer, state->font, ms_str,
                    x, y, 1.0f, 0.0f, CP_GREEN);
  }
  temp_end(temp);
}
