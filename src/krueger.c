#include "krueger_base.h"
#include "krueger_platform.h"
#include "krueger_random.h"
#include "krueger_shared.h"

#include "krueger_base.c"
#include "krueger_platform.c"
#include "krueger_random.c"

// TODO:
// - more enemy type
// - work on enemies's shooting, movement pattern
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
load_bmp(Arena *arena, Temp scratch, String8 filepath) {
  Image result = {0};
  void *file_data = platform_read_entire_file(scratch.arena, filepath);
  if (file_data) {
    Bmp_File_Header *bmp_file = (Bmp_File_Header *)file_data;
    if (bmp_file->type == cstr_encode("BM")) {
      Bmp_Info_Header *bmp_info = (Bmp_Info_Header *)((u8 *)bmp_file + sizeof(Bmp_File_Header));
      u32 width = bmp_info->image_width;
      u32 height = bmp_info->image_height;
      uxx size = width*height*(bmp_info->bits_per_pixel/8);
      u32 *pixels = arena_push(arena, size);
      u32 *bmp_data = (u32 *)((u8 *)file_data + bmp_file->data_offset);
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
} Tilemap;

typedef struct {
  char *chars;
  Tilemap tilemap;
} Font;

typedef enum {
  ENEMY_NULL,
  ENEMY_SHIP,
  ENEMY_BOMB,
} Enemy_Type;

typedef enum {
  PARTICLE_STAR,
  PARTICLE_EXPLOSION,
  PARTICLE_FLASH,
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
  f32 time;
} Particle;

typedef enum {
  ENEMY_BEHAVIOR_UNDEFINED,
  ENEMY_BEHAVIOR_CHARGE,
  ENEMY_BEHAVIOR_FLYING,
  ENEMY_BEHAVIOR_ATTACK,
} Enemy_Behavior;

typedef struct Entity Entity;
struct Entity {
  Entity *next;
  Entity *prev;
  b32 is_alive;

  Enemy_Type type;
  Enemy_Behavior behavior;
  Vector2 target_position;
  f32 wait;

  f32 flash_t;
  f32 invul_t;
  f32 shoot_t;
  f32 charge_time;

  f32 flash_cd;
  f32 invul_cd;
  f32 shoot_cd;

  u32 hp;
  f32 speed;
  Vector2 size;
  Vector2 pos;
  Vector2 dpos;

  u32 sprite_index;
  u32 sprite_w;
  u32 sprite_h;

  u32 flame_sprite_index;
  u32 flame_sprite_frame_count;
  f32 flame_sprite_frame_frequency;
  u32 next_flame_sprite_frame;
  Vector2 flame_sprite_offset;
  f32 flame_sprite_frame_time;

  f32 particle_emission_cooldown;
  f32 particle_emission_time;
  u32 next_particle;
  Particle particles[256];
};

typedef enum {
  SCREEN_MENU,
  SCREEN_GAME,
  SCREEN_OVER,
  SCREEN_WIN,
} Screen_State;

typedef struct String8_Node String8_Node;
struct String8_Node {
  String8_Node *next;
  String8 string;
};

typedef struct String8_List String8_List;
struct String8_List {
  String8_Node *first;
  String8_Node *last;
  u32 node_count;
};

internal String8_Node *
str8_list_push(Arena *arena, String8_List *list, String8 string) {
  String8_Node *result = push_array(arena, String8_Node, 1);
  sll_queue_push(list->first, list->last, result);
  list->node_count += 1;
  result->string = string;
  return(result);
}

internal String8_Node *
str8_list_push_fmt(Arena *arena, String8_List *list, char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  String8 string = push_str8_fmt(arena, fmt);
  String8_Node *result = str8_list_push(arena, list, string);
  va_end(args);
  return(result);
}

internal void
str8_list_pop(String8_List *list) {
  list->node_count -= 1;
  sll_queue_pop(list->first, list->last);
}

typedef struct {
  Random_Series entropy;
  Arena main_arena;
  Arena temp_arena;
  Image font_image;
  Image sprites_image;
  Tilemap font_tilemap;
  Tilemap sprites_tilemap;

  Font font;
  f32 tile_size;
  Vector2 screen_size;

  Screen_State screen_state;
  f32 time;
  f32 blink_frequency;
  f32 lock_input_cooldown;
  f32 lock_input_time;

  f32 player_max_shoot_r;
  f32 player_shoot_r;
  f32 player_dshoot_r;
  u32 player_hp_sprite_index;
  u32 player_shoot_side;
  f32 player_win_position_t;
  Entity player;
  
  // NOTE: bullet manager
  Arena bullets_arena;
  Entity *first_bullet;
  Entity *last_bullet;
  Entity *first_free_bullet;
  u32 total_bullet_count;
  u32 bullet_count;
 
  // NOTE: enemy/wave manager
  u32 wave_index;
  f32 wave_warn_cd;
  f32 wave_warn_t;
  f32 enemy_spawn_cd;
  f32 enemy_spawn_t;
  f32 enemy_attack_cooldown;
  f32 enemy_attack_time;
  b32 enemies_spawned;
  u32 wave_enemy_count;
  u32 remaining_enemy_count;
  u32 next_enemy;

  u32 max_enemy_count;
  Entity *enemies;
  
  // NOTE: particle manager
  u32 star_particle_count;
  u32 explosion_particle_count;
  u32 particle_count;

  Particle *particles;
  Particle *star_particles;
  Particle *explosion_particles;
  u32 next_explosion_particle;

  // NOTE: string info
  b32 draw_time_info;
  b32 draw_debug_info;
  String8_List time_string_list;
  String8_List debug_string_list;
} Game_State;

/////////////////////////
// NOTE: Helper Functions

internal Tilemap
make_tilemap(Image image,
             u32 tile_w, u32 tile_h,
             u32 tile_count_x, u32 tile_count_y) {
  Tilemap result = {
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
tilemap_get_tile(Tilemap tilemap, u32 tile_index) {
  assert(tile_index < tilemap.max_tile_count);
  u32 tile_w = tilemap.tile_w;
  u32 tile_h = tilemap.tile_h;
  u32 tile_x = tilemap.tile_w*(tile_index%tilemap.tile_count_x);
  u32 tile_y = tilemap.tile_h*(tile_index/tilemap.tile_count_x);
  Image result = image_scissor(tilemap.image, tile_x, tile_y, tile_w, tile_h);
  return(result);
}

internal Image
tilemap_get_wide_tile(Tilemap tilemap, u32 tile_index, u32 tile_count_x, u32 tile_count_y) {
  assert(tile_index < tilemap.max_tile_count);
  u32 tile_w = tilemap.tile_w*tile_count_x;
  u32 tile_h = tilemap.tile_h*tile_count_y;
  u32 tile_x = tilemap.tile_w*(tile_index%tilemap.tile_count_x);
  u32 tile_y = tilemap.tile_h*(tile_index/tilemap.tile_count_x);
  Image result = image_scissor(tilemap.image, tile_x, tile_y, tile_w, tile_h);
  return(result);
}

internal Font
make_font(char *chars, Tilemap tilemap) {
  Font result = {
    .chars = chars,
    .tilemap = tilemap,
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
draw_rect_fill2(Image dst, Vector2 min, Vector2 max, u32 color) {
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
draw_circle_line2(Image dst, Vector2 position, f32 r, u32 color) {
  draw_circle_line(dst, (s32)position.x, (s32)position.y, (s32)r, color);
}

internal void
draw_circle_fill2(Image dst, Vector2 position, f32 r, u32 color) {
  draw_circle_fill(dst, (s32)position.x, (s32)position.y, r, color);
}

internal void
draw_texture(Image dst, Image src, s32 x, s32 y) {
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
draw_texture_f32(Image dst, Image src, f32 x, f32 y) {
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
draw_texture2(Image dst, Image src, Vector2 position) {
  draw_texture_f32(dst, src, position.x, position.y);
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
draw_texture_shader_f32(Image dst, Image src, f32 x, f32 y,
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

internal void
draw_texture_shader2(Image dst, Image src, Vector2 position,
                     pixel_shader_proc *callback, void *user_data) {
  draw_texture_shader_f32(dst, src, position.x, position.y, callback, user_data);
}

internal Vector2
align_position(Image image, Vector2 position, Vector2 align) {
  align.x = align.x*image.width;
  align.y = align.y*image.height;
  position = vector2_sub(position, align);
  return(position);
}

internal void
draw_sprite(Image draw_buffer, Tilemap tilemap,
            u32 sprite_index, u32 sprite_w, u32 sprite_h,
            Vector2 position, Vector2 align, Vector2 offset) {
  Image sprite = tilemap_get_wide_tile(tilemap, sprite_index, sprite_w, sprite_h);
  position = align_position(sprite, position, align);
  position = vector2_add(position, offset);
  draw_texture2(draw_buffer, sprite, position);
}

internal void
draw_sprite_shader(Image draw_buffer, Tilemap sprites_tilemap,
                   u32 sprite_index, u32 sprite_w, u32 sprite_h,
                   Vector2 position, Vector2 align, Vector2 offset,
                   pixel_shader_proc *shader, void *user_data) {
  Image sprite = tilemap_get_wide_tile(sprites_tilemap, sprite_index, sprite_w, sprite_h);
  position = align_position(sprite, position, align);
  position = vector2_add(position, offset);
  draw_texture_shader2(draw_buffer, sprite, position, shader, user_data);
}

internal void
draw_text(Image dst, Font font, String8 text,
          s32 x, s32 y, u32 color) {
  s32 glyph_w = font.tilemap.tile_w;
  s32 glyph_h = font.tilemap.tile_h;
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
    Image sprite = tilemap_get_tile(font.tilemap, char_index);
    Vector2 pos = make_vector2((f32)glyph_x, (f32)glyph_y);
    draw_texture_shader2(dst, sprite, pos, shader_color_blend, &color);
    glyph_x += glyph_w;
  }
}

internal void
draw_text_align(Image dst, Font font, String8 text,
                s32 x, s32 y, f32 align_x, f32 align_y,
                u32 color) {
  s32 len = (s32)text.len;
  x = x - (s32)floor_f32(align_x*(font.tilemap.tile_w*len));
  y = y - (s32)floor_f32(align_y*font.tilemap.tile_h);
  draw_text(dst, font, text, x, y, color);
}

///////////////////////
// NOTE: Game Functions

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
emit_explosion_particles(Game_State *state, Vector2 position) {
  {
    Particle *particle = state->explosion_particles + state->next_explosion_particle++;
    if (state->next_explosion_particle >= state->explosion_particle_count) {
      state->next_explosion_particle = 0;
    }
    particle->type = PARTICLE_EXPLOSION;
    particle->color = CP_WHITE;
    particle->speed = 0.0f;
    particle->age = 0.0f;
    particle->max_age = 0.5f;
    particle->radius = 10.0f;
    particle->dradius = 24.0f;
    particle->pos = position;
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
    particle->max_age = 1.0f;
    particle->radius = random_range(&state->entropy, 1.0f, 4.0f);
    particle->dradius = 10.0f;
    particle->fric = 480.0f;
    particle->speed = random_range(&state->entropy, 100.0f, 125.0f);
    particle->pos = position;
    particle->dpos = make_vector2(random_bilateral(&state->entropy),
                                  random_bilateral(&state->entropy));
  }
}

internal void
emit_flash_particles(Game_State *state) {
  Particle *particle = state->explosion_particles + state->next_explosion_particle++;
  if (state->next_explosion_particle >= state->explosion_particle_count) {
    state->next_explosion_particle = 0;
  }
  mem_zero_struct(particle);
  particle->type = PARTICLE_FLASH;
  particle->color = CP_WHITE;
  // particle->life = 1.0f;
  // particle->dlife = -0.7f;
  particle->life = 0.8f;
  particle->dlife = -1.0f;
}

internal void
player_emit_flame_particles(Entity *entity, Game_State *state, Clock *time) {
  entity->particle_emission_time += time->dt_sec;
  if (entity->particle_emission_time >= entity->particle_emission_cooldown) {
    Particle *particle = entity->particles + entity->next_particle++;
    if (entity->next_particle >= array_count(entity->particles)) {
      entity->next_particle = 0;
    }

    particle->color = CP_BLUE;

    particle->life = 10.0f;
    particle->dlife = -30.0f;

    particle->radius = 2.5f;
    particle->dradius = -9.0f;

    particle->speed = 50.0f;
    particle->dspeed = -150.0f;

    particle->pos = entity->pos;
    particle->pos.y += state->tile_size*0.7f;
    particle->dpos = make_vector2(random_range(&state->entropy, -0.2f, 0.2f), 1.0f);

    entity->particle_emission_time = 0.0f;
  }
}

internal void
player_update_and_draw_particles(Entity *entity, Clock *time, Image draw_buffer) {
  for (u32 particle_index = 0;
       particle_index < array_count(entity->particles);
       ++particle_index) {
    Particle *particle = entity->particles + particle_index;
    if (particle->life > 0.0f) {
      particle->life += particle->dlife*time->dt_sec;

      particle->radius += particle->dradius*time->dt_sec;
      if (particle->radius <= 0.0f) particle->radius = 0.0f;

      particle->speed += particle->dspeed*time->dt_sec;
      if (particle->speed <= 0.0f) particle->speed = 0.0f;

      Vector2 dpos = vector2_mul(particle->dpos, particle->speed*time->dt_sec);
      particle->pos = vector2_add(particle->pos, dpos);

      draw_circle_fill2(draw_buffer, particle->pos,
                        particle->radius, particle->color);
    }
  }
}

internal void
enemy_emit_flame_particles(Entity *entity, Game_State *state, Clock *time) {
  entity->particle_emission_time += time->dt_sec;
  if (entity->particle_emission_time >= entity->particle_emission_cooldown) {
    Particle *particle = entity->particles + entity->next_particle++;
    if (entity->next_particle >= array_count(entity->particles)) {
      entity->next_particle = 0;
    }

    particle->color = CP_RED;

    particle->life = 10.0f;
    particle->dlife = -30.0f;

    particle->radius = 2.5f;
    particle->dradius = -9.0f;

    particle->speed = 25.0f;
    particle->dspeed = -150.0f;

    particle->pos = entity->pos;
    particle->pos.y -= state->tile_size*0.6f;
    particle->dpos = make_vector2(random_range(&state->entropy, -0.2f, 0.2f), -1.0f);

    entity->particle_emission_time = 0.0f;
  }
}

internal void
enemy_update_and_draw_particles(Entity *entity, Clock *time, Image draw_buffer) {
  for (u32 particle_index = 0;
       particle_index < array_count(entity->particles);
       ++particle_index) {
    Particle *particle = entity->particles + particle_index;
    if (particle->life > 0.0f) {
      particle->life += particle->dlife*time->dt_sec;

      particle->radius += particle->dradius*time->dt_sec;
      if (particle->radius <= 0.0f) particle->radius = 0.0f;

      particle->speed += particle->dspeed*time->dt_sec;
      if (particle->speed <= 0.0f) particle->speed = 0.0f;

      Vector2 dpos = vector2_mul(particle->dpos, particle->speed*time->dt_sec);
      particle->pos = vector2_add(particle->pos, dpos);

      draw_circle_fill2(draw_buffer, particle->pos,
                        particle->radius, particle->color);
    }
  }
}

#define WAVE_W 9
#define WAVE_H 4

internal void
enemy_behave(Entity *entity, Game_State *state, Clock *time) {
  if (entity->wait > 0.0f) {
    entity->wait += -1.0f*time->dt_sec;
    return;
  }
  switch (entity->behavior) {
    case ENEMY_BEHAVIOR_UNDEFINED: {
    } break;
    case ENEMY_BEHAVIOR_CHARGE: {
      entity->dpos.x = cos_f32(entity->charge_time*32.0f)*0.4f;

      Vector2 velocity = vector2_mul(entity->dpos, entity->speed*time->dt_sec);
      entity->pos.x += velocity.x;

      entity->charge_time += time->dt_sec;
      if (entity->charge_time > 0.7f) {
        entity->charge_time = 0.0f;
        entity->dpos.x = 0.0f;
        entity->behavior = ENEMY_BEHAVIOR_ATTACK;
      }
    } break;
    case ENEMY_BEHAVIOR_FLYING: {
      entity->pos.x += (entity->target_position.x - entity->pos.x)/40.0f;
      entity->pos.y += (entity->target_position.y - entity->pos.y)/40.0f;
      if (abs_t(f32, entity->target_position.y - entity->pos.y) < 1.0f) {
        entity->pos.y = entity->target_position.y;
        entity->behavior = ENEMY_BEHAVIOR_UNDEFINED;
      }
    } break;
    case ENEMY_BEHAVIOR_ATTACK: {
      Vector2 velocity = vector2_mul(entity->dpos, entity->speed*time->dt_sec);
      entity->pos = vector2_add(entity->pos, velocity);
      f32 limit = 128.0f + WAVE_H*entity->size.y*1.5f;
      if (entity->pos.y >= limit) {
        entity->pos.y = -entity->target_position.y;
        entity->behavior = ENEMY_BEHAVIOR_FLYING;
      }
    } break;
    default: {
      invalid_path;
    } break;
  }
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

internal Entity *
init_player(Game_State *state) {
  state->player_max_shoot_r = 5.0f;
  state->player_shoot_r = 0.0f;
  state->player_dshoot_r = -80.0f;
  state->player_hp_sprite_index = 32;

  Entity *entity = &state->player;
  entity->is_alive = true;

  entity->invul_t = 0.0f;
  entity->shoot_t = 0.0f;

  entity->invul_cd = 1.5f;
  entity->shoot_cd = 0.1f;

  entity->hp = 3;
  entity->speed = 80.0f;
  entity->size = make_vector2(state->tile_size*0.2f, state->tile_size*0.2f);
  entity->pos = make_vector2(state->screen_size.x/2.0f,
                             state->screen_size.y - state->screen_size.y/4.0f);
  entity->dpos = make_vector2(0.0f, 0.0f);

  entity->sprite_index = 1;
  entity->sprite_w = 1;
  entity->sprite_h = 1;

  entity->flame_sprite_index = 16;
  entity->flame_sprite_frame_count = 5;
  entity->flame_sprite_frame_frequency = 0.04f;
  entity->next_flame_sprite_frame = entity->flame_sprite_index;
  entity->flame_sprite_offset = make_vector2(0.0f, 8.0f);
  entity->flame_sprite_frame_time = 0.0f;

  entity->particle_emission_cooldown = 0.15f;
  entity->particle_emission_time = 0.0f;

  return(entity);
}

internal Entity *
bullet_alloc(Game_State *state) {
  Entity *result = state->first_free_bullet;
  if (result) {
    sll_stack_pop(state->first_free_bullet);
  } else {
    result = push_array(&state->bullets_arena, Entity, 1);
    state->total_bullet_count += 1;
  }
  mem_zero_struct(result);
  dll_push_back(state->first_bullet, state->last_bullet, result);
  state->bullet_count += 1;
  return(result);
}

internal void
bullet_release(Game_State *state, Entity *bullet) {
  state->bullet_count -= 1;
  dll_remove(state->first_bullet, state->last_bullet, bullet);
  sll_stack_push(state->first_free_bullet, bullet);
}

internal void
simulate_bullets(Game_State *state, Clock *time) {
  for (Entity *bullet = state->first_bullet; bullet != 0; bullet = bullet->next) {
    if (bullet->is_alive) {
      Vector2 dpos = vector2_mul(bullet->dpos, bullet->speed*time->dt_sec);
      bullet->pos = vector2_add(bullet->pos, dpos);
      if (bullet->pos.y <= -state->tile_size) {
        bullet->is_alive = false;
      }
    }
  }
}

internal void
remove_dead_bullets(Game_State *state) {
  for (Entity *bullet = state->last_bullet; bullet != 0; bullet = bullet->prev) {
    if (!bullet->is_alive) {
      bullet_release(state, bullet);
    }
  }
}

internal void
draw_bullets(Game_State *state, Image draw_buffer) {
  for (Entity *bullet = state->first_bullet; bullet != 0; bullet = bullet->next) {
    draw_sprite(draw_buffer, state->sprites_tilemap,
                bullet->sprite_index, bullet->sprite_w, bullet->sprite_h,
                bullet->pos, vec2(0.5f, 0.5f), vec2(0.0f, 0.0f));
  }
}

internal Entity *
add_enemy(Game_State *state, Enemy_Type type) {
  Entity *enemy = state->enemies + state->next_enemy++;
  if (state->next_enemy >= state->max_enemy_count) {
    state->next_enemy = 0;
  }
  enemy->is_alive = true;
  enemy->type = type;
  enemy->flash_t = 0.0f;
  enemy->flash_cd = 0.05f;
  switch (enemy->type) {
    case ENEMY_SHIP: {
      enemy->behavior = ENEMY_BEHAVIOR_FLYING;
      enemy->hp = 7;
      enemy->speed = 64.0f;
      enemy->size = make_vector2(state->tile_size, state->tile_size);
      enemy->dpos = make_vector2(0.0f, 1.0f);

      enemy->sprite_index = 64;
      enemy->sprite_w = 1;
      enemy->sprite_h = 1;
      
      enemy->flame_sprite_index = 48;
      enemy->flame_sprite_frame_count = 4;
      enemy->flame_sprite_frame_frequency = 0.05f;
      enemy->next_flame_sprite_frame = enemy->flame_sprite_index + 
        random_choice(&state->entropy, enemy->flame_sprite_frame_count);
      enemy->flame_sprite_offset = make_vector2(0.0f, -8.0f);
      enemy->flame_sprite_frame_time = 0.0f;

      enemy->particle_emission_cooldown = 0.15f;
      enemy->particle_emission_time = 0.0f;
    } break;
    case ENEMY_BOMB: {
      enemy->hp = 2;
      enemy->speed = 32.0f;
      enemy->size = make_vector2(state->tile_size, state->tile_size);
      enemy->dpos = make_vector2(0.0f, 0.0f);

      enemy->sprite_index = 54;
      enemy->sprite_w = 1;
      enemy->sprite_h = 2;
    } break;
    default: {
      invalid_path;
    }
  }
  state->wave_enemy_count += 1;
  state->remaining_enemy_count += 1;
  return(enemy);
}

internal void
damage_player(Game_State *state, Entity *entity, u32 damage) {
  entity->invul_t = entity->invul_cd;
  if (entity->hp > 0) entity->hp -= damage;
  if (entity->hp <= 0) {
    mem_zero_struct(entity);
    state->lock_input_time = state->lock_input_cooldown + state->time;
    state->screen_state = SCREEN_OVER;
  }
}

internal void
damage_enemy(Game_State *state, Entity *entity, u32 damage) {
  entity->flash_t = entity->flash_cd;
  if (entity->hp > 0) entity->hp -= damage;
  if (entity->hp <= 0) {
    emit_explosion_particles(state, entity->pos);
    mem_zero_struct(entity);
    state->remaining_enemy_count -= 1;
  }
}

internal void
destroy_bullet(Game_State *state, Entity *entity) {
  mem_zero_struct(entity);
}

// TODO:
internal void
update_entity_flame(Clock *time, Entity *entity) {
  entity->flame_sprite_frame_time += time->dt_sec;
  if (entity->flame_sprite_frame_time >= entity->flame_sprite_frame_frequency) {
    entity->next_flame_sprite_frame += 1;
    if (entity->next_flame_sprite_frame >
        (entity->flame_sprite_index + entity->flame_sprite_frame_count - 1)) {
      entity->next_flame_sprite_frame = entity->flame_sprite_index;
    }
    entity->flame_sprite_frame_time = 0.0f;
  }
}

global u32 waves[6][WAVE_W*WAVE_H] = {
  [0] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 1, 0, 1, 0, 1, 0, 1, 0,
    0, 0, 1, 0, 0, 0, 1, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0,
  },

  [1] = {
    0, 1, 0, 1, 0, 1, 0, 1, 0,
    0, 1, 0, 0, 1, 0, 0, 1, 0,
    0, 0, 1, 0, 0, 0, 1, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0,
  },

  [2] = {
    0, 0, 0, 1, 0, 1, 0, 0, 0,
    1, 0, 0, 1, 0, 1, 0, 0, 1,
    0, 1, 0, 0, 0, 0, 0, 1, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0,
  },

  [3] = {
    0, 0, 0, 0, 1, 0, 0, 0, 0,
    0, 1, 0, 1, 0, 1, 0, 1, 0,
    1, 0, 1, 0, 1, 0, 1, 0, 1,
    0, 1, 0, 0, 0, 0, 0, 1, 0,
  },

  [4] = {
    1, 0, 0, 0, 1, 0, 0, 0, 1,
    1, 0, 0, 0, 1, 0, 0, 0, 1,
    0, 0, 1, 0, 0, 0, 1, 0, 0,
    1, 0, 0, 1, 1, 1, 0, 0, 1,
  },

  [5] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 1, 0, 0, 0, 0, 0, 1, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0,
  },
};

internal void
reset_wave(Game_State *state) {
  state->wave_warn_t = state->time + state->wave_warn_cd;
  state->enemy_spawn_t = state->wave_warn_t + state->enemy_spawn_cd;
  state->enemy_attack_time = 0.0f;
  state->enemy_attack_cooldown = random_range(&state->entropy, 0.8f, 1.8f);
  state->enemies_spawned = false;
  state->wave_enemy_count = 0;
  state->remaining_enemy_count = 0;
  state->next_enemy = 0;
  mem_zero_typed(state->enemies, state->max_enemy_count);
}

internal void
init_wave(Game_State *state) {
  state->wave_index = 0;
  state->wave_warn_cd = 2.5f;
  state->enemy_spawn_cd = 0.4f;
  reset_wave(state);
}

internal void
next_wave(Game_State *state) {
  reset_wave(state);
  state->wave_index += 1;
  if (state->wave_index >= 6) {
    state->lock_input_time = state->lock_input_cooldown + state->time;
    state->player_win_position_t = 0.0f;
    state->screen_state = SCREEN_WIN;
  }
}

shared_function
KRUEGER_INIT_PROC(krueger_init) {
  assert(sizeof(Game_State) <= memory->memory_size);
  Game_State *state = cast(Game_State *) memory->memory_ptr;

  Date_Time date_time = platform_get_date_time();
  Dense_Time dense_time = dense_time_from_date_time(date_time);
  state->entropy = random_seed(cast(u32) dense_time);

  uxx half_memory_size = memory->memory_size/2;
  uxx perm_memory_size = half_memory_size - sizeof(Game_State);
  uxx temp_memory_size = half_memory_size;

  u8 *perm_memory_ptr = cast(u8 *) memory->memory_ptr + sizeof(Game_State);
  u8 *temp_memory_ptr = memory->memory_ptr + half_memory_size;

  state->main_arena = make_arena(perm_memory_ptr, perm_memory_size);
  state->temp_arena = make_arena(temp_memory_ptr, temp_memory_size);
  
  Temp scratch = temp_begin(&state->temp_arena);
  state->font_image     = load_bmp(&state->main_arena, scratch,
                                   str8_lit("../res/pico8_font.bmp"));
  state->sprites_image  = load_bmp(&state->main_arena, scratch,
                                   str8_lit("../res/sprites.bmp"));
  temp_end(scratch);

  state->font_tilemap = make_tilemap(state->font_image, 4, 6, 16, 6);
  state->sprites_tilemap = make_tilemap(state->sprites_image, 8, 8, 16, 16);

  char *chars = " !\"#$%&'()*+,-./"
                "0123456789:;<=>?"
                "@ABCDEFGHIJKLMNO"
                "PQRSTUVWXYZ[\\]^_"
                "`abcdefghijklmno"
                "pqrstuvwxyz{|}~ ";
  state->font = make_font(chars, state->font_tilemap);
  state->tile_size = 8.0f;
  state->screen_size = make_vector2(cast(f32) back_buffer->width,
                                    cast(f32) back_buffer->height);
  state->screen_state = SCREEN_MENU;

  state->time = 0.0f;
  state->blink_frequency = 12.0f;
  state->lock_input_cooldown = 1.0f;
  state->lock_input_time = state->lock_input_cooldown;
  
  // NOTE: bullet manager
  u32 max_bullet_count = 32;
  uxx bullets_memory_size = sizeof(Entity)*max_bullet_count;
  state->bullets_arena = make_subarena(&state->main_arena, bullets_memory_size);
  
  state->remaining_enemy_count = 0;
  state->max_enemy_count = 256;
  state->enemies = push_array(&state->main_arena, Entity, state->max_enemy_count);
  mem_zero_typed(state->enemies, state->max_enemy_count);

  state->star_particle_count = 32;
  state->explosion_particle_count = 256;
  state->particle_count = (state->star_particle_count + 
                           state->explosion_particle_count);
  state->particles = push_array(&state->main_arena, Particle, state->particle_count);
  mem_zero_typed(state->particles, state->particle_count);
  
  s32 offset = 0;
  state->star_particles = state->particles;
  offset += state->star_particle_count;
  state->explosion_particles = state->particles + offset;

  f32 min_star_speed = 32.0f;
  f32 max_star_speed = 64.0f;

  for (u32 i = 0; i < state->star_particle_count; ++i) {
    Particle *particle = state->star_particles + i;
    particle->color = CP_DARK_BLUE;
    particle->radius = cast(f32) random_choice(&state->entropy, 3);
    particle->speed = random_range(&state->entropy, min_star_speed, max_star_speed);
    if (particle->speed >= 0.5f*(min_star_speed + max_star_speed)) {
      particle->color = CP_DARK_GRAY;
    }
    particle->pos.x = random_unilateral(&state->entropy)*back_buffer->width;
    particle->pos.y = random_unilateral(&state->entropy)*back_buffer->height;
    particle->dpos = make_vector2(0.0f, -1.0f);
  }

  state->draw_time_info = true;
  state->draw_debug_info = true;
}

shared_function
KRUEGER_FRAME_PROC(krueger_frame) {
  Game_State *state = cast(Game_State *) memory->memory_ptr;
  state->time += time->dt_sec;

  Digital_Button *kbd = input->kbd;
  if (kbd[KEY_F1].pressed) state->draw_time_info = !state->draw_time_info;
  if (kbd[KEY_F2].pressed) state->draw_debug_info = !state->draw_debug_info;

  Image draw_buffer = *back_buffer; 
  image_fill(draw_buffer, CP_BLACK);
 
#if 0
  s32 tile_count_x = cast(s32) (draw_buffer.width/state->tile_size);
  s32 tile_count_y = cast(s32) (draw_buffer.height/state->tile_size);

  for (s32 y = 0; y < tile_count_y; ++y) {
    for (s32 x = 0; x < tile_count_x; ++x) {
      s32 x0 = cast(s32) (x*state->tile_size);
      s32 y0 = cast(s32) (y*state->tile_size);
      s32 x1 = cast(s32) (x0 + state->tile_size);
      s32 y1 = cast(s32) (y0 + state->tile_size);
      u32 color = CP_DARK_GRAY;
      if ((x + y) % 2 == 0) {
        color = CP_DARK_BLUE;
      }
      draw_rect_fill(draw_buffer, x0, y0, x1, y1, color);
    }
  }
#endif

  // NOTE: update and draw particles
  for (u32 particle_index = 0;
       particle_index < state->particle_count;
       ++particle_index) {
    Particle *particle = state->particles + particle_index;
    switch (particle->type) {
      case PARTICLE_STAR: {
        if (state->time < state->wave_warn_t ||
            state->screen_state == SCREEN_WIN) {
          particle->pos.y += 2.5f*particle->speed*time->dt_sec;
        } else {
          particle->pos.y += particle->speed*time->dt_sec;
        }
        if (particle->pos.y - particle->radius > (f32)draw_buffer.height) {
          particle->pos.x = random_unilateral(&state->entropy)*draw_buffer.width;
          particle->pos.y = -particle->radius;
        }

        if (particle->radius <= 1.0f) {
          draw_circle_line2(draw_buffer, particle->pos,
                            particle->radius, particle->color);
        } else {
          draw_circle_fill2(draw_buffer, particle->pos,
                            particle->radius, particle->color);
        }
      } break;
      case PARTICLE_EXPLOSION: {
        if (particle->radius > 0.0f) {
          Vector2 dpos = vector2_mul(particle->dpos, particle->speed*time->dt_sec);
          particle->pos = vector2_add(particle->pos, dpos);

          particle->speed -= particle->fric*time->dt_sec;
          if (particle->speed < 0.0f) particle->speed = 0.0f;

          particle->fric += (particle->fric*0.9f)*time->dt_sec;

          particle->age += 2.0f*time->dt_sec;
          if (particle->age > 0.15f) particle->color = CP_YELLOW;
          if (particle->age > 0.27f) particle->color = CP_ORANGE;
          if (particle->age > 0.43f) particle->color = CP_RED;
          if (particle->age > 0.7f) particle->color = CP_DARK_RED;
          if (particle->age > 0.9f) particle->color = CP_DARK_BROWN;
          if (particle->age > particle->max_age) {
            particle->radius -= particle->dradius*time->dt_sec;
          }

          draw_circle_fill2(draw_buffer, particle->pos,
                            particle->radius, particle->color);
        }
      } break;
      case PARTICLE_FLASH: {
        if (particle->life > 0.0f) {
          particle->life += particle->dlife*time->dt_sec;
          particle->time += time->dt_sec;
          if (sin_f32(particle->time*48.0f) > 0.5f) {
            draw_rect_fill(draw_buffer, 0, 0, draw_buffer.width, draw_buffer.height,
                           particle->color);
          }
        }
      } break;
    }
  }

  switch (state->screen_state) {
    case SCREEN_MENU: {
      s32 bw = draw_buffer.width;
      s32 bh = draw_buffer.height;

      { // NOTE: draw title
        s32 x = bw/2;
        s32 y = bh/3;
        draw_text_align(draw_buffer, state->font, str8_lit("STARFIGHTER"),
                        x, y, 0.5f, 0.5f, CP_RED);
      }
      
      if (state->time > state->lock_input_time) {
        if (kbd[KEY_Z].pressed) {
          init_player(state);
          init_wave(state);
          state->screen_state = SCREEN_GAME;
        }

        String8 text = str8_lit("press [z] to start");

        s32 x = bw/2;
        s32 y = bh - bh/3;

        if (sin_f32(state->time*state->blink_frequency) > -0.5f) {
          draw_text_align(draw_buffer, state->font, text, x, y, 0.5f, 0.5f, CP_DARK_BROWN);
        } else {
          draw_text_align(draw_buffer, state->font, text, x, y, 0.5f, 0.5f, CP_GRAY);
        }
      }
    } break;

    case SCREEN_GAME: {
      // NOTE: update player
      if (state->player.is_alive) {
        if (state->player.invul_t > 0.0f) {
          state->player.invul_t -= 1.0f*time->dt_sec;
        } else {
          player_emit_flame_particles(&state->player, state, time);
        }
        player_update_and_draw_particles(&state->player, time, draw_buffer);
        update_entity_flame(time, &state->player);

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
        if (state->player.shoot_t <= 0.0f) {
          if (kbd[KEY_Z].is_down) {
            state->player.shoot_t = state->player.shoot_cd;
            state->player_shoot_r = state->player_max_shoot_r;
            Entity *bullet = bullet_alloc(state);
            bullet->is_alive = true;
            bullet->speed = 380.0f;
            bullet->size = make_vector2(state->tile_size, state->tile_size);
            bullet->pos = state->player.pos;
            bullet->dpos = make_vector2(0.0f, -1.0f);
            bullet->sprite_index = 4;
            bullet->sprite_w = 1;
            bullet->sprite_h = 1;
            bullet->pos.x += (state->player_shoot_side++ % 2 == 0) ? 1.0f : -1.0f;
          }
        } else {
          state->player.shoot_t -= 1.0f*time->dt_sec;
        }
      }

      simulate_bullets(state, time);
      draw_bullets(state, draw_buffer);

      // NOTE: go to next wave when all enemies died
      if (state->enemies_spawned && state->remaining_enemy_count == 0) {
        next_wave(state);
      }

      // NOTE: add enemies
      if (state->time > state->wave_warn_t &&
          state->time > state->enemy_spawn_t &&
          !state->enemies_spawned) {
        state->enemies_spawned = true;
        u32 *wave = waves[state->wave_index];
        for (u32 y = 0; y < WAVE_H; ++y) {
          for (u32 x = 0; x < WAVE_W; ++x) {
            Enemy_Type entity_type = wave[y*WAVE_W + x];
            if (entity_type != ENEMY_NULL) {
              Entity *enemy = add_enemy(state, entity_type);
              enemy->wait = (f32)x*0.0f;

              f32 gap_x = enemy->size.x*3.0f;
              f32 gap_y = enemy->size.y*1.5f;

              f32 offset_x = 0.5f*(draw_buffer.width - WAVE_W*gap_x + gap_x);
              f32 offset_y = -1.0f*(WAVE_H*gap_y);

              enemy->pos.x = offset_x + x*gap_x;
              enemy->pos.y = offset_y + y*gap_y;

              gap_x = enemy->size.x*1.5f;
              offset_x = 0.5f*(draw_buffer.width - WAVE_W*gap_x + gap_x);
              offset_y = state->tile_size*2.0f;

              enemy->target_position = make_vector2(offset_x + x*gap_x,
                                                    offset_y + y*gap_y);
            }
          }
        }
      }

      // NOTE: change enemy behavior randomly
      if (state->enemies_spawned) {
        state->enemy_attack_time += time->dt_sec;
        if (state->enemy_attack_time > state->enemy_attack_cooldown) {
          u32 enemy_index = random_choice(&state->entropy, state->wave_enemy_count);
          Entity *enemy = state->enemies + enemy_index;
          if (enemy->is_alive && enemy->behavior == ENEMY_BEHAVIOR_UNDEFINED) {
            enemy->behavior = ENEMY_BEHAVIOR_CHARGE;
          }
          state->enemy_attack_time = 0.0f;
          state->enemy_attack_cooldown = random_range(&state->entropy, 1.4f, 2.0f);
        }
      }

      // NOTE: update and draw enemies
      for (u32 enemy_index = 0;
           enemy_index < state->max_enemy_count;
           ++enemy_index) {
        Entity *enemy = state->enemies + enemy_index;
        if (enemy->is_alive) {
          switch (enemy->type) {
            case ENEMY_SHIP: {
              enemy_behave(enemy, state, time);
              enemy_emit_flame_particles(enemy, state, time);
              enemy_update_and_draw_particles(enemy, time, draw_buffer);
              update_entity_flame(time, enemy);

              pixel_shader_proc *shader = 0;
              u32 user_data = 0;

              if (enemy->flash_t > 0.0f) {
                enemy->flash_t -= 1.0f*time->dt_sec;
                shader = shader_replace_color;
                user_data = CP_WHITE;
              }

              draw_sprite(draw_buffer, state->sprites_tilemap,
                          enemy->next_flame_sprite_frame, 1, 1,
                          enemy->pos, vec2(0.5f, 0.5f), vec2(0.0f, -8.0f));
              draw_sprite_shader(draw_buffer, state->sprites_tilemap,
                                 enemy->sprite_index, enemy->sprite_w, enemy->sprite_h,
                                 enemy->pos, vec2(0.5f, 0.5f), vec2(0.0f, 0.0f),
                                 shader, &user_data);
            } break;
            case ENEMY_BOMB: {
              pixel_shader_proc *shader = 0;
              u32 user_data = 0;

              if (enemy->flash_t > 0.0f) {
                enemy->flash_t -= 1.0f*time->dt_sec;
                shader = shader_replace_color;
                user_data = CP_WHITE;
              }

              draw_sprite_shader(draw_buffer, state->sprites_tilemap,
                                 enemy->sprite_index, enemy->sprite_w, enemy->sprite_h,
                                 enemy->pos, vec2(0.5f, 0.5f), vec2(0.0f, 0.0f),
                                 shader, &user_data);
            } break;
          }
        }
      }

      // NOTE: player x enemy collision
      if (state->player.is_alive) {
        for (u32 enemy_index = 0;
             enemy_index < state->max_enemy_count;
             ++enemy_index) {
          Entity *enemy = state->enemies + enemy_index;
          if (enemy->is_alive) {
            if (entity_collide(state->player, *enemy)) {
              if (state->player.is_alive &&
                  state->player.invul_t <= 0.0f) {
                // emit_flash_particles(state);
                damage_enemy(state, enemy, enemy->hp);
                damage_player(state, &state->player, 1);
              }
            }
          }
        }
      }

      for (Entity *bullet = state->first_bullet; bullet != 0; bullet = bullet->next) {
        if (bullet->is_alive) {
          for (u32 enemy_index = 0;
               enemy_index < state->max_enemy_count;
               ++enemy_index) {
            Entity *enemy = state->enemies + enemy_index;
            if (enemy->is_alive) {
              if (entity_collide(*bullet, *enemy)) {
                damage_enemy(state, enemy, 1);
                bullet->is_alive = false;
              }
            }
          }
        }
      }

      remove_dead_bullets(state);

      // NOTE: draw player
      if (state->player.is_alive) {
        { // NOTE: draw player shoot effect
          if (state->player_shoot_r > 0.0f) {
            state->player_shoot_r += state->player_dshoot_r*time->dt_sec;
          }
          if (state->player_shoot_r < 0.0f) state->player_shoot_r = 0.0f;
          Vector2 pos = state->player.pos;
          f32 gap = 1.0f;
          pos.x += (state->player_shoot_side%2 == 0) ? -gap : gap;
          pos.y -= 3.0f;
          draw_circle_fill2(draw_buffer, pos, state->player_shoot_r, CP_WHITE);
        }

        if (state->player.invul_t <= 0.0f) {
          draw_sprite(draw_buffer, state->sprites_tilemap,
                      state->player.next_flame_sprite_frame, 1, 1,
                      state->player.pos, vec2(0.5f, 0.5f), vec2(0.0f, 8.0f));
          draw_sprite(draw_buffer, state->sprites_tilemap, state->player.sprite_index,
                      state->player.sprite_w, state->player.sprite_h,
                      state->player.pos, vec2(0.5f, 0.5f), vec2(0.0f, 0.0f));
        } else {
          if (sin_f32(thousand(1.0f)*state->time) <= 0.0f) {
            draw_sprite(draw_buffer, state->sprites_tilemap, state->player.sprite_index,
                        state->player.sprite_w, state->player.sprite_h,
                        state->player.pos, vec2(0.5f, 0.5f), vec2(0.0f, 0.0f));
          }
        }
      }

      // NOTE: draw ui
      s32 bw = draw_buffer.width;
      s32 bh = draw_buffer.height;

      { // NOTE: draw enemy count
        Temp temp = temp_begin(&state->temp_arena);
        String8 text = push_str8_fmt(temp.arena, "%d/%d",
                                     state->remaining_enemy_count,
                                     state->wave_enemy_count);
        s32 x = bw - 1;
        s32 y = 1;
        draw_text_align(draw_buffer, state->font, text, x, y, 1.0f, 0.0f, CP_RED);
        temp_end(temp);
      }

      // NOTE: draw player hp
      for (u32 hp_index = 0;
           hp_index < state->player.hp;
           ++hp_index) {
        Image sprite = tilemap_get_tile(state->sprites_tilemap,
                                        state->player_hp_sprite_index);
        Vector2 position = make_vector2(0.0f, cast(f32) (bh - sprite.height));
        Vector2 offset = make_vector2(hp_index*sprite.width + 1.0f, -1.0f);
        draw_sprite(draw_buffer, state->sprites_tilemap,
                    state->player_hp_sprite_index, 1, 1,
                    position, vec2(0.0f, 0.0f), offset);
      }

      // NOTE: draw wave warn
      if (state->time < state->wave_warn_t) {
        Temp temp = temp_begin(&state->temp_arena);
        String8 text = push_str8_fmt(temp.arena, "WAVE %d", state->wave_index + 1);
        s32 x = bw/2;
        s32 y = bh/5;
        if (sin_f32(state->time*state->blink_frequency) > -0.5f) {
          draw_text_align(draw_buffer, state->font, text,
                          x, y, 0.5f, 0.5f, CP_DARK_RED);
        } else {
          draw_text_align(draw_buffer, state->font, text,
                          x, y, 0.5f, 0.5f, CP_RED);
        }
        temp_end(temp);
      }
    } break;

    case SCREEN_OVER: {
      s32 bw = draw_buffer.width;
      s32 bh = draw_buffer.height;

      { // NOTE: draw game over
        s32 x = bw/2;
        s32 y = bh/3;
        draw_text_align(draw_buffer, state->font, str8_lit("GAME OVER"),
                        x, y, 0.5f, 0.5f, CP_RED);
      }

      if (state->time > state->lock_input_time) {
        if (kbd[KEY_Z].pressed) {
          state->lock_input_time = state->lock_input_cooldown + state->time;
          state->screen_state = SCREEN_MENU;
        }

        String8 text0 = str8_lit("press [z] to go");
        String8 text1 = str8_lit("back to menu");

        s32 th = state->font.tilemap.tile_h;

        s32 x = bw/2;
        s32 y = bh - bh/3;

        if (sin_f32(state->time*state->blink_frequency) > -0.5f) {
          draw_text_align(draw_buffer, state->font, text0,
                          x, y, 0.5f, 0.5f, CP_DARK_BROWN);
          y += th;
          draw_text_align(draw_buffer, state->font, text1,
                          x, y, 0.5f, 0.5f, CP_DARK_BROWN);
        } else {
          draw_text_align(draw_buffer, state->font, text0,
                          x, y, 0.5f, 0.5f, CP_GRAY);
          y += th;
          draw_text_align(draw_buffer, state->font, text1,
                          x, y, 0.5f, 0.5f, CP_GRAY);
        }
      }
    } break;

    case SCREEN_WIN: {
      state->player_win_position_t += time->dt_sec*0.02f;
      if (state->player_win_position_t >= 1.0f) {
        state->player_win_position_t = 1.0f;
      }

      s32 bw = draw_buffer.width;
      s32 bh = draw_buffer.height;

      state->player.pos = vector2_lerp(state->player.pos, vec2(bw*0.5f, bh*0.5f),
                                       state->player_win_position_t);

      player_emit_flame_particles(&state->player, state, time);
      player_update_and_draw_particles(&state->player, time, draw_buffer);
      update_entity_flame(time, &state->player);

      draw_sprite(draw_buffer, state->sprites_tilemap,
                  state->player.next_flame_sprite_frame, 1, 1,
                  state->player.pos, vec2(0.5f, 0.5f), vec2(0.0f, 8.0f));
      draw_sprite(draw_buffer, state->sprites_tilemap, 1, 1, 1,
                  state->player.pos, vec2(0.5f, 0.5f), vec2(0.0f, 0.0f));

      { // NOTE: draw win
        s32 x = bw/2;
        s32 y = bh/3;
        draw_text_align(draw_buffer, state->font, str8_lit("YOU WIN!"),
                        x, y, 0.5f, 0.5f, CP_BLUE);
      }

      if (state->time > state->lock_input_time) {
        if (kbd[KEY_Z].pressed) {
          state->lock_input_time = state->lock_input_cooldown + state->time;
          state->screen_state = SCREEN_MENU;
        }

        String8 text0 = str8_lit("press [z] to go");
        String8 text1 = str8_lit("back to menu");

        s32 th = state->font.tilemap.tile_h;

        s32 x = bw/2;
        s32 y = bh - bh/3;

        if (sin_f32(state->time*state->blink_frequency) > -0.5f) {
          draw_text_align(draw_buffer, state->font, text0,
                          x, y, 0.5f, 0.5f, CP_DARK_BROWN);
          y += th;
          draw_text_align(draw_buffer, state->font, text1,
                          x, y, 0.5f, 0.5f, CP_DARK_BROWN);
        } else {
          draw_text_align(draw_buffer, state->font, text0,
                          x, y, 0.5f, 0.5f, CP_GRAY);
          y += th;
          draw_text_align(draw_buffer, state->font, text1,
                          x, y, 0.5f, 0.5f, CP_GRAY);
        }
      }
    } break;
  }

  // NOTE: draw time info
  if (state->draw_time_info) {
    Temp temp = temp_begin(&state->temp_arena);

    f32 fps = million(1.0f)/time->_dt_us;
    f32 ms = time->_dt_us/thousand(1.0f);
    
    String8_List *list = &state->time_string_list;
    str8_list_push_fmt(temp.arena, list, "%.2f FPS", fps);
    str8_list_push_fmt(temp.arena, list, "%.2f MS ", ms);
    
    s32 bw = draw_buffer.width;
    s32 bh = draw_buffer.height;

    s32 tw = state->font.tilemap.tile_w;
    s32 th = state->font.tilemap.tile_h;

    s32 x = bw - 2;
    s32 y = bh - list->node_count*th - 2;
    
    for (String8_Node *node = list->first; node != 0; node = node->next) {
      String8 string = node->string;
      s32 x0 = x - cast(s32) string.len*tw;
      s32 y0 = y;
      s32 x1 = x + 1;
      s32 y1 = y0 + th + 1;
      draw_rect_fill(draw_buffer, x0, y0, x1, y1, CP_BLACK);
      draw_text_align(draw_buffer, state->font, string, x, y, 1.0f, 0.0f, CP_DARK_GRAY);
      str8_list_pop(list);
      y += th;
    }

    temp_end(temp);
  }
  
  // NOTE: draw debug info
  if (state->draw_debug_info) {
    Temp temp = temp_begin(&state->temp_arena);

    String8_List *list = &state->debug_string_list;
    str8_list_push_fmt(temp.arena, list, "TBC %d", state->total_bullet_count);
    str8_list_push_fmt(temp.arena, list, " BC %d", state->bullet_count);

    s32 tw = state->font.tilemap.tile_w;
    s32 th = state->font.tilemap.tile_h;

    s32 x = 1;
    s32 y = 1;

    for (String8_Node *node = list->first; node != 0; node = node->next) {
      String8 string = node->string;
      s32 x0 = x;
      s32 y0 = y;
      s32 x1 = x0 + cast(s32) string.len*tw + 1;
      s32 y1 = y0 + th + 1;
      draw_rect_fill(draw_buffer, x0, y0, x1, y1, CP_BLACK);
      draw_text_align(draw_buffer, state->font, string, x, y, 0.0f, 0.0f, CP_DARK_GRAY);
      str8_list_pop(list);
      y += th;
    }

    temp_end(temp);
  }
}
