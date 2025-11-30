#define BUILD_ENTRY_POINT 0

#include "krueger_base.h"
#include "krueger_platform.h"
#include "krueger_keycode.h"

#include "krueger_random.h"
#include "krueger_shared.h"

#include "krueger_base.c"
#include "krueger_platform.c"

#include "krueger_random.c"

// TODO:
// final boss
// more enemies/levels?
// particle emitter
// asset manager:
// - sprite group
// - sprite animation system
// audio

////////////////
// NOTE: Globals

#define TILE_SIZE 8.0f

#define WAVE_MAX 6
#define WAVE_W 9
#define WAVE_H 4
#define LAST_WAVE (WAVE_MAX - 1)

global u32 waves[WAVE_MAX][WAVE_W*WAVE_H] = {
  [0] = {
    0, 0, 0, 2, 0, 2, 0, 0, 0,
    0, 1, 0, 0, 1, 0, 0, 1, 0,
    0, 0, 1, 0, 0, 0, 1, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0,
  },

  [1] = {
    0, 0, 0, 0, 2, 0, 0, 0, 0,
    2, 0, 0, 0, 0, 0, 0, 0, 2,
    0, 1, 0, 1, 0, 1, 0, 1, 0,
    0, 0, 0, 0, 1, 0, 0, 0, 0,
  },

  [2] = {
    0, 0, 0, 2, 0, 2, 0, 0, 0,
    1, 0, 0, 1, 0, 1, 0, 0, 1,
    0, 2, 0, 0, 1, 0, 0, 2, 0,
    0, 0, 1, 0, 0, 0, 1, 0, 0,
  },

  [3] = {
    0, 1, 0, 0, 2, 0, 0, 1, 0,
    0, 0, 1, 0, 0, 0, 1, 0, 0,
    2, 0, 0, 2, 0, 2, 0, 0, 2,
    0, 1, 0, 0, 1, 0, 0, 1, 0,
  },

  [4] = {
    0, 2, 0, 0, 1, 0, 0, 2, 0,
    1, 0, 0, 2, 0, 2, 0, 0, 1,
    0, 0, 1, 0, 1, 0, 1, 0, 0,
    0, 2, 0, 1, 0, 1, 0, 2, 0,
  },

  [LAST_WAVE] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 4, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0,
  },
};

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
load_bmp(Arena *arena, String8 file_path) {
  Image result = {0};
  Temp scratch = scratch_begin(&arena, 1);
  void *file_data = platform_read_entire_file(scratch.arena, file_path);
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
    log_error("%s: failed to read file: %s", __func__, file_path);
  }
  scratch_end(scratch);
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
  String8 chars;
  Tilemap tilemap;
} Font;

typedef enum {
  PARTICLE_STAR,
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
  Vector2 position;
  Vector2 velocity;
  f32 age;
  f32 max_age;
  f32 fric;
  f32 time;
} Particle;

typedef enum {
  ENEMY_NULL,
  ENEMY_BOMBER,
  ENEMY_ASSASSIN,
  ENEMY_MINE,
  ENEMY_BOSS,
} Enemy_Type;

typedef enum {
  ENEMY_BEHAVIOR_FLYING,
  ENEMY_BEHAVIOR_IDLE,
  ENEMY_BEHAVIOR_CHARGE,
  ENEMY_BEHAVIOR_ATTACK,

  ENEMY_BEHAVIOR_BOSS1,
  ENEMY_BEHAVIOR_BOSS2,
  ENEMY_BEHAVIOR_BOSS3,
  ENEMY_BEHAVIOR_BOSS4,
  ENEMY_BEHAVIOR_BOSS5,
} Enemy_Behavior;

typedef struct Entity Entity;
struct Entity {
  Entity *next;
  Entity *prev;

  b32 is_alive;
  b32 is_player_friendly;
  
  u32 max_hp;
  u32 hp;
  f32 speed;
  Vector2 velocity;
  Vector2 position;
  Vector2 size;

  f32 shoot_time;
  f32 shoot_cooldown;

  // NOTE: player exclusive
  // {
  f32 invencible_time;
  f32 invencible_countdown;
  // }
  
  // NOTE: enemy exclusive 
  // {
  Enemy_Type type;
  Enemy_Behavior behavior;
  
  Vector2 position0;
  Vector2 position1;
  Vector2 target_position;
  Vector2 shoot_direction;

  f32 time;
  f32 transition_time;

  f32 flash_time;
  f32 flash_countdown;

  f32 charge_countdown;
  f32 charge_time;

  f32 attack_time;
  f32 attack_countdown;

  u32 bullet_per_attack;
  u32 bullet_fired;
  u32 attack_count;
  // }

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

///////////////////////
// NOTE: Entity Manager

typedef struct {
  Entity *first;
  Entity *last;
  u32 total_count;
  u32 count;
} Entity_List;

typedef struct {
  Arena *arena;
  Entity_List list;
  Entity *first_free;
} Entity_Manager;

internal Entity_Manager *
entity_manager_alloc(Arena *arena, u32 max_entity_count) {
  Entity_Manager *result = push_array(arena, Entity_Manager, 1);
  mem_zero_struct(result);
  void *base = arena_push(arena, sizeof(Entity)*max_entity_count);
  result->arena = arena_alloc(.base = base);
  return(result);
}

internal Entity *
entity_alloc(Entity_Manager *manager) {
  Entity *result = manager->first_free;
  if (result) {
    stack_pop(manager->first_free);
  } else {
    result = push_array(manager->arena, Entity, 1);
    manager->list.total_count += 1;
  }
  mem_zero_struct(result);
  dll_push_back(manager->list.first, manager->list.last, result);
  manager->list.count += 1;
  return(result);
}

internal void
entity_release(Entity_Manager *manager, Entity *entity) {
  manager->list.count -= 1;
  dll_remove(manager->list.first, manager->list.last, entity);
  stack_push(manager->first_free, entity);
}

internal void
remove_dead_entities(Entity_Manager *manager) {
  for (Entity *entity = manager->list.last; entity != 0; entity = entity->prev) {
    if (!entity->is_alive) {
      entity_release(manager, entity);
    }
  }
}

internal void
remove_all_entities(Entity_Manager *manager) {
  for (Entity *entity = manager->list.last; entity != 0; entity = entity->prev) {
    entity_release(manager, entity);
  }
}

///////////////////
// NOTE: Game State

typedef enum {
  SCREEN_MENU,
  SCREEN_GAME,
  SCREEN_OVER,
  SCREEN_WIN,
} Screen_State;

typedef struct {
  Random_Series entropy;

  Arena *main_arena;
  Arena *temp_arena;

  Image font_image;
  Image sprites_image;

  Tilemap font_tilemap;
  Tilemap sprites_tilemap;

  Font font;
  Image draw_buffer;
  
  Screen_State screen_state;
  b32 pause;
  f32 time;
  f32 shake_oscillation;
  f32 blink_frequency;
  f32 lock_input_time;
  f32 lock_input_cooldown;
  f32 record_time;

  f32 player_max_shoot_r;
  f32 player_shoot_r;
  f32 player_dshoot_r;
  u32 player_shoot_side;
  f32 player_win_position_t;
  u32 bomb_count;

  Entity *player;
  Entity_Manager *enemy_manager;
  Entity_Manager *bullet_manager;

  // NOTE: Particle Manager
  Particle star_particles[32];
  
  u32 next_explosion_particle;
  Particle explosion_particles[1024];

  u32 next_flash_particle;
  Particle flash_particles[4];

  // NOTE: Wave System
  // TODO: clean this up
  u32 wave_index;
  f32 wave_warn_cooldown;
  f32 wave_warn_time;
  f32 enemy_spawn_cooldown;
  f32 enemy_spawn_time;
  f32 enemy_attack_cooldown;
  f32 enemy_attack_time;
  b32 enemies_spawned;
  u32 wave_enemy_count;
  u32 wave_remaining_enemy_count;

  // NOTE: Info
  b32 draw_debug_info;
  b32 draw_time_info;
  String8_List debug_string_list;
  String8_List time_string_list;
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
make_font(String8 chars, Tilemap tilemap) {
  Font result = {
    .chars = chars,
    .tilemap = tilemap,
  };
  return(result);
}

///////////////////////
// NOTE: Draw Functions

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
    u32 char_index = (u32)cstr_index_of((char *)font.chars.str, c);
    assert(font.chars.str[char_index] == c);
    Image sprite = tilemap_get_tile(font.tilemap, char_index);
    Vector2 position = make_vector2((f32)glyph_x, (f32)glyph_y);
    draw_texture_shader2(dst, sprite, position, shader_color_blend, &color);
    glyph_x += glyph_w;
  }
}

internal Vector2
measure_text_size(Font font, String8 text) {
  Vector2 result = {
    result.x = (f32)font.tilemap.tile_w*text.len,
    result.y = (f32)font.tilemap.tile_h,
  };
  return(result);
}

internal void
draw_text_align(Image dst, Font font, String8 text,
                Vector2 position, Vector2 align,
                u32 color) {
  Vector2 text_size = measure_text_size(font, text);
  align = vector2_hadamard(align, text_size);
  position = vector2_sub(position, align);
  draw_text(dst, font, text, (s32)position.x, (s32)position.y, color);
}

/////////////////
// TODO: CLEAN UP

internal void enemy_behave(Entity *entity, Game_State *state, Clock *time);
internal void update_entity_flame(Clock *time, Entity *entity);
internal void change_screen(Game_State *state, Screen_State screen);

internal void
emit_explosion(Game_State *state, Vector2 position) {
  {
    Particle *particle = state->explosion_particles + state->next_explosion_particle++;
    if (state->next_explosion_particle >= array_count(state->explosion_particles)) {
      state->next_explosion_particle = 0;
    }
    particle->color = CP_WHITE;
    particle->speed = 0.0f;
    particle->age = 0.0f;
    particle->max_age = 0.5f;
    particle->radius = 10.0f;
    particle->dradius = 24.0f;
    particle->position = position;
    particle->velocity = make_vector2(random_bilateral(&state->entropy),
                                  random_bilateral(&state->entropy));
  }

  for (u32 i = 0; i < 32; ++i) {
    Particle *particle = state->explosion_particles + state->next_explosion_particle++;
    if (state->next_explosion_particle >= array_count(state->explosion_particles)) {
      state->next_explosion_particle = 0;
    }
    particle->color = CP_WHITE;
    particle->age = random_range(&state->entropy, 0.0f, 0.5f);
    particle->max_age = 1.0f;
    particle->radius = random_range(&state->entropy, 1.0f, 4.0f);
    particle->dradius = 10.0f;
    particle->fric = 480.0f;
    particle->speed = random_range(&state->entropy, 100.0f, 125.0f);
    particle->position = position;
    particle->velocity = make_vector2(random_bilateral(&state->entropy),
                                  random_bilateral(&state->entropy));
  }
}

internal void
emit_flash(Game_State *state, u32 color) {
  Particle *particle = state->flash_particles + state->next_flash_particle++;
  if (state->next_flash_particle >= array_count(state->flash_particles)) {
    state->next_flash_particle = 0;
  }
  particle->color = color;
  particle->life = 0.5f;
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

    particle->position = entity->position;
    particle->position.y += TILE_SIZE*0.7f;
    particle->velocity = make_vector2(random_range(&state->entropy, -0.2f, 0.2f), 1.0f);

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

      Vector2 velocity = vector2_mul(particle->velocity, particle->speed*time->dt_sec);
      particle->position = vector2_add(particle->position, velocity);

      draw_circle_fill2(draw_buffer, particle->position,
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

    particle->position = entity->position;
    particle->position.y -= entity->sprite_h*TILE_SIZE*0.5f + 1.0f;
    particle->velocity = make_vector2(random_range(&state->entropy, -0.2f, 0.2f), -1.0f);

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

      Vector2 velocity = vector2_mul(particle->velocity, particle->speed*time->dt_sec);
      particle->position = vector2_add(particle->position, velocity);

      draw_circle_fill2(draw_buffer, particle->position,
                        particle->radius, particle->color);
    }
  }
}

internal void
draw_enemies(Image draw_buffer, Entity_Manager *manager, Tilemap tilemap, Clock *time) {
  for (Entity *enemy = manager->list.first;
       enemy != 0;
       enemy = enemy->next) {
    if (enemy->is_alive) {
      enemy_update_and_draw_particles(enemy, time, draw_buffer);

      pixel_shader_proc *shader = 0;
      u32 user_data = 0;
      
      if (enemy->flash_countdown > 0.0f) {
        shader = shader_replace_color;
        user_data = CP_WHITE;
      }

      draw_sprite(draw_buffer, tilemap,
                  enemy->next_flame_sprite_frame, 1, 1,
                  enemy->position,
                  vec2(0.5f, 0.0f),
                  vec2(0.0f, -1.0f*TILE_SIZE - enemy->sprite_h*TILE_SIZE*0.5f));
      draw_sprite_shader(draw_buffer, tilemap,
                         enemy->sprite_index, enemy->sprite_w, enemy->sprite_h,
                         enemy->position, vec2(0.5f, 0.5f), vec2(0.0f, 0.0f),
                         shader, &user_data);
    }
  }
}

internal void
draw_bullets(Image draw_buffer, Entity_Manager *manager, Tilemap tilemap) {
  for (Entity *bullet = manager->list.first; bullet != 0; bullet = bullet->next) {
    draw_sprite(draw_buffer, tilemap,
                bullet->sprite_index, bullet->sprite_w, bullet->sprite_h,
                bullet->position, vec2(0.5f, 0.5f), vec2(0.0f, 0.0f));
  }
}

////////////////////////////
// NOTE: Entity Manipulation

internal b32
entity_collide(Entity a, Entity b) {
  Vector2 half_size = vector2_mul(a.size, 0.5f);
  f32 a_left    = a.position.x - half_size.x;
  f32 a_top     = a.position.y - half_size.y;
  f32 a_right   = a.position.x + half_size.x;
  f32 a_bottom  = a.position.y + half_size.y;

  half_size = vector2_mul(b.size, 0.5f);
  f32 b_left    = b.position.x - half_size.x;
  f32 b_top     = b.position.y - half_size.y;
  f32 b_right   = b.position.x + half_size.x;
  f32 b_bottom  = b.position.y + half_size.y;

  b32 result = true;
  if ((a_top > b_bottom) ||
      (b_top > a_bottom) ||
      (a_left > b_right) ||
      (b_left > a_right)) {
    result = false;
  }
  return(result);
}

internal void
linear_move(Entity *entity, Clock *time) {
  Vector2 velocity = vector2_mul(entity->velocity, entity->speed*time->dt_sec);
  entity->position = vector2_add(entity->position, velocity);
}

internal void
countdown(f32 *countdown, Clock *time) {
  if (*countdown > 0.0f) *countdown -= time->dt_sec;
  if (*countdown < 0.0f) *countdown = 0.0f;
}

internal b32
keep_point_between_range(Vector2 *point, Vector2 min, Vector2 max) {
  b32 result = false;
  if (point->x < min.x) {
    point->x = min.x;
    result = true;
  }
  if (point->y < min.y) {
    point->y = min.y;
    result = true;
  }
  if (point->x > max.x) {
    point->x = max.x;
    result = true;
  }
  if (point->y > max.y) {
    point->y = max.y;
    result = true;
  }
  return(result);
}

internal Entity *
player_init(Game_State *state) {
  state->player_max_shoot_r = 5.0f;
  state->player_shoot_r = 0.0f;
  state->player_dshoot_r = -80.0f;
  state->bomb_count = 2;

  Entity *player = state->player;
  player->is_alive = true;
  player->is_player_friendly = true;

  player->hp = 3;
  player->speed = 70.0f;
  player->velocity = make_vector2(0.0f, 0.0f);
  player->position.x = state->draw_buffer.width*0.5f;
  player->position.y = state->draw_buffer.height - state->draw_buffer.height*0.25f;
  player->size = make_vector2(1.0f, 1.0f);
  
  // player->shoot_time = 0.08f;
  player->shoot_time = 0.09f;
  player->shoot_cooldown = 0.0f;

  player->invencible_time = 3.0f;
  player->invencible_countdown = 0.0f;

  player->sprite_index = 1;
  player->sprite_w = 1;
  player->sprite_h = 1;

  player->flame_sprite_index = 16;
  player->flame_sprite_frame_count = 5;
  player->flame_sprite_frame_frequency = 0.04f;
  player->next_flame_sprite_frame = player->flame_sprite_index;
  player->flame_sprite_offset = make_vector2(0.0f, 8.0f);
  player->flame_sprite_frame_time = 0.0f;

  player->particle_emission_cooldown = 0.15f;
  player->particle_emission_time = 0.0f;

  return(player);
}

internal b32
hit_player(Game_State *state, Entity *entity, u32 damage) {
  b32 result = false;
  if (entity->is_alive &&
      entity->invencible_countdown <= 0.0f) {
    if (entity->hp > 0) {
      emit_flash(state, CP_WHITE);
      state->shake_oscillation += 4.0f;
      entity->hp -= damage;
    }
    if (entity->hp <= 0) {
      mem_zero_struct(entity);
      change_screen(state, SCREEN_OVER);
    }
    entity->invencible_countdown = entity->invencible_time;
    result = true;
  }
  return(result);
}

internal Entity *
add_enemy(Game_State *state, Enemy_Type type) {
  Entity *enemy = entity_alloc(state->enemy_manager);

  enemy->is_alive = true;
  enemy->is_player_friendly = false;

  enemy->type = type;
  enemy->behavior = ENEMY_BEHAVIOR_FLYING;

  enemy->flash_time = 0.03f;
  enemy->flash_countdown = 0.0f;

  switch (enemy->type) {
    case ENEMY_BOMBER: {
      enemy->hp = 15;
      enemy->speed = 100.0f;
      enemy->size = make_vector2(TILE_SIZE, TILE_SIZE);
      enemy->velocity = make_vector2(0.0f, 1.0f);

      // enemy->shoot_time = 0.1f;
      enemy->shoot_time = 0.15f;
      enemy->shoot_cooldown = 0.0f;

      enemy->charge_time = 0.8f;
      enemy->charge_countdown = 0.0f;

      enemy->sprite_index = 64;
      enemy->sprite_w = 1;
      enemy->sprite_h = 1;
      
    } break;
    case ENEMY_ASSASSIN: {
      enemy->hp = 13;
      enemy->speed = 40.0f;
      enemy->size = make_vector2(TILE_SIZE, TILE_SIZE);
      enemy->velocity = make_vector2(0.0f, 0.0f);

      enemy->shoot_time = 0.13f;
      enemy->shoot_cooldown = 0.0f;

      enemy->charge_time = 1.0f;
      enemy->charge_countdown = enemy->charge_time;
      
      enemy->bullet_fired = 0;
      enemy->bullet_per_attack = 3;

      enemy->sprite_index = 66;
      enemy->sprite_w = 1;
      enemy->sprite_h = 1;
    } break;
    case ENEMY_MINE: {
      enemy->hp = 10;
      enemy->speed = 30.0f;
      enemy->size = make_vector2(TILE_SIZE, TILE_SIZE);
      enemy->velocity = make_vector2(0.0f, 0.0f);

      enemy->sprite_index = 68;
      enemy->sprite_w = 1;
      enemy->sprite_h = 1;
    } break;
    case ENEMY_BOSS: {
      enemy->max_hp = 300;
      enemy->hp = enemy->max_hp;
      enemy->speed = 40.0f;
      enemy->size = make_vector2(TILE_SIZE, TILE_SIZE);
      enemy->velocity = make_vector2(0.0f, 0.0f);

      enemy->shoot_time = 1.0f;
      enemy->shoot_cooldown = 0.0f;

      enemy->sprite_index = 91;
      enemy->sprite_w = 2;
      enemy->sprite_h = 2;
    } break;
    default: {
      invalid_path;
    }
  }

  enemy->flame_sprite_index = 48;
  enemy->flame_sprite_frame_count = 4;
  enemy->flame_sprite_frame_frequency = 0.05f;
  enemy->next_flame_sprite_frame = enemy->flame_sprite_index + 
    random_choice(&state->entropy, enemy->flame_sprite_frame_count);
  enemy->flame_sprite_offset = make_vector2(0.0f, -8.0f);
  enemy->flame_sprite_frame_time = 0.0f;

  enemy->particle_emission_cooldown = 0.15f;
  enemy->particle_emission_time = 0.0f;

  state->wave_enemy_count += 1;
  state->wave_remaining_enemy_count += 1;

  return(enemy);
}

internal void
hit_enemy(Game_State *state, Entity *entity, u32 damage) {
  entity->flash_countdown = entity->flash_time;
  if (entity->hp > 0) entity->hp -= damage;
  if (entity->hp <= 0) {
    emit_explosion(state, entity->position);
    state->shake_oscillation += 1.0f;
    entity->is_alive = false;
    state->wave_remaining_enemy_count -= 1;
  }
}

internal void
enemy_behave(Entity *entity, Game_State *state, Clock *time) {
  switch (entity->behavior) {
    case ENEMY_BEHAVIOR_FLYING: {
      entity->position.x += (entity->target_position.x - entity->position.x)/20.0f;
      entity->position.y += (entity->target_position.y - entity->position.y)/20.0f;
      f32 dy = abs_t(f32, entity->target_position.y - entity->position.y);
      if (dy < 1.0f) {
        entity->time = state->time;
        entity->position = entity->target_position;
        entity->behavior = ENEMY_BEHAVIOR_IDLE;

        if (entity->type == ENEMY_BOSS) {
          entity->velocity.x = 1.0f;
          entity->position0.x = state->draw_buffer.width*0.25f;
          entity->position1.x = state->draw_buffer.width - state->draw_buffer.width*0.25f;
          entity->behavior = ENEMY_BEHAVIOR_BOSS1;
        }
      }
    } break;

    case ENEMY_BEHAVIOR_BOSS1: {
      linear_move(entity, time);

      b32 change = false;
      if (entity->position.x <= entity->position0.x) {
        entity->position.x = entity->position0.x;
        change = true;
      }

      if (entity->position.x >= entity->position1.x) {
        entity->position.x = entity->position1.x;
        change = true;
      }

      if (change) {
        entity->velocity.x *= -1.0f;
        entity->behavior = ENEMY_BEHAVIOR_BOSS2;
      }

    } break;
    case ENEMY_BEHAVIOR_BOSS2: {
      entity->shoot_time = 0.3f;
      countdown(&entity->shoot_cooldown, time);
      if (entity->shoot_cooldown <= 0.0f) {
        f32 offset = random_bilateral(&state->entropy)*10.0f;
        for (u32 i = 0; i < 3; ++i) {
          Entity *bullet = entity_alloc(state->bullet_manager);
          bullet->is_alive = true;
          bullet->is_player_friendly = false;
          bullet->speed = 80.0f;
          bullet->position = entity->position;
          f32 theta_radians = radians_pi32(75.0f + offset) + radians_pi32(i*15.0f);
          bullet->velocity.x = cos_f32(theta_radians);
          bullet->velocity.y = sin_f32(theta_radians);
          bullet->size = make_vector2(4.0f, 4.0f);
          bullet->sprite_index = 97;
          bullet->sprite_w = 1;
          bullet->sprite_h = 1;
          entity->shoot_cooldown = entity->shoot_time;
        }
        entity->bullet_fired += 1;
      }
      if (entity->bullet_fired == 6) {
        entity->attack_count += 1;
        entity->bullet_fired = 0;
        if (entity->attack_count < 2) {
          entity->behavior = ENEMY_BEHAVIOR_BOSS1;
        } else {
          entity->attack_count = 0;
          entity->position0.x = entity->position.x;
          entity->position1.x = state->draw_buffer.width*0.5f;
          entity->behavior = ENEMY_BEHAVIOR_BOSS3;
        }
      }
    } break;

    case ENEMY_BEHAVIOR_BOSS3: {
      linear_move(entity, time);
      f32 delta = abs_t(f32, entity->position1.x - entity->position.x);
      if (delta < 1.0f) {
        entity->behavior = ENEMY_BEHAVIOR_BOSS4;
      }
    } break;

    case ENEMY_BEHAVIOR_BOSS4: {
      entity->shoot_time = 0.1f;
      countdown(&entity->shoot_cooldown, time);
      if (entity->shoot_cooldown <= 0.0f) {
        f32 offset = 10.0f*entity->bullet_fired;
        for (u32 i = 0; i < 8; ++i) {
          Entity *bullet = entity_alloc(state->bullet_manager);
          bullet->is_alive = true;
          bullet->is_player_friendly = false;
          bullet->speed = 30.0f;
          bullet->position = entity->position;
          f32 theta_radians = radians_pi32(i*45.0f + offset);
          bullet->velocity.x = cos_f32(theta_radians);
          bullet->velocity.y = sin_f32(theta_radians);
          bullet->size = make_vector2(2.0f, 2.0f);
          bullet->sprite_index = 96;
          bullet->sprite_w = 1;
          bullet->sprite_h = 1;
          entity->shoot_cooldown = entity->shoot_time;
        }
        entity->bullet_fired += 1;
      }
      if (entity->bullet_fired > 36) {
        entity->bullet_fired = 0;
        entity->velocity.x = 1.0f;
        entity->position0.x = state->draw_buffer.width*0.25f;
        entity->position1.x = state->draw_buffer.width - state->draw_buffer.width*0.25f;
        entity->behavior = ENEMY_BEHAVIOR_BOSS5;
      }
    } break;

    case ENEMY_BEHAVIOR_BOSS5: {
      if (entity->position.x != entity->position1.x) {
        linear_move(entity, time);
        f32 delta = abs_t(f32, entity->position1.x - entity->position.x);
        if (delta < 1.0f) {
          entity->position.x = entity->position1.x;
        }
      } else {
        entity->shoot_time = 0.5f;
        countdown(&entity->shoot_cooldown, time);
        if (entity->shoot_cooldown <= 0.0f) {
          Vector2 delta = vector2_sub(state->player->position, entity->position);
          f32 theta_radians = atan2f(delta.y, delta.x);

          Entity *enemy = add_enemy(state, ENEMY_MINE);
          enemy->hp = 8;
          enemy->speed = 75.0f;
          enemy->position = entity->position;
          enemy->velocity.x = cos_f32(theta_radians);
          enemy->velocity.y = sin_f32(theta_radians);
          enemy->behavior = ENEMY_BEHAVIOR_IDLE;

          entity->shoot_cooldown = entity->shoot_time;
          entity->bullet_fired += 1;
        }

        if (entity->bullet_fired > 1) {
          entity->velocity.x = 1.0f;
          entity->position0.x = state->draw_buffer.width*0.25f;
          entity->position1.x = state->draw_buffer.width - state->draw_buffer.width*0.25f;
          entity->bullet_fired = 0;
          entity->behavior = ENEMY_BEHAVIOR_BOSS1;
        }
      }
    } break;

    case ENEMY_BEHAVIOR_IDLE: {
      switch (entity->type) {
        case ENEMY_ASSASSIN: {
          countdown(&entity->charge_countdown, time);
          if (entity->charge_countdown) {
            linear_move(entity, time);

            Vector2 min = make_vector2(0.0f, 0.0f);
            Vector2 max = make_vector2((f32)state->draw_buffer.width,
                                       (f32)state->draw_buffer.height);

            if (keep_point_between_range(&entity->position, min, max)) {
              entity->velocity.x *= -1.0f;
            }
          }
        } break;
        case ENEMY_MINE: {
          Vector2 delta = vector2_sub(state->player->position, entity->position);
          f32 distance = abs_t(f32, square(delta.x) + square(delta.y));
          if (distance < square(TILE_SIZE*6.0f)) {
            entity->behavior = ENEMY_BEHAVIOR_ATTACK;
          }
          entity->velocity = vector2_normalize(delta);
          linear_move(entity, time);
        } break;
      }
    } break;
    case ENEMY_BEHAVIOR_CHARGE: {
      switch (entity->type) {
        case ENEMY_BOMBER: {
          entity->velocity.x = cos_f32(entity->charge_countdown*60.0f)*0.5f;

          Vector2 velocity = vector2_mul(entity->velocity, entity->speed*time->dt_sec);
          entity->position.x += velocity.x;

          entity->charge_countdown += time->dt_sec;
          if (entity->charge_countdown > entity->charge_time) {
            entity->charge_countdown = 0.0f;
            entity->velocity.x = 0.0f;
            entity->behavior = ENEMY_BEHAVIOR_ATTACK;
            if (entity->position.x < state->player->position.x) {
              entity->shoot_direction.x = 1.0f;
            } else {
              entity->shoot_direction.x = -1.0f;
            }
          }
        } break;
        case ENEMY_ASSASSIN: {
          if (entity->position.x < state->player->position.x) {
            entity->velocity.x = 1.0f;
          } else {
            entity->velocity.x = -1.0f;
          }
          linear_move(entity, time);
          f32 dx = abs_t(f32, entity->position.x - state->player->position.x);
          if (dx < 16.0f) {
            entity->behavior = ENEMY_BEHAVIOR_ATTACK;
          }
        } break;
      }
    } break;
    case ENEMY_BEHAVIOR_ATTACK: {
      switch (entity->type) {
        case ENEMY_BOMBER: {
          linear_move(entity, time);

          f32 limit = 128.0f + WAVE_H*entity->size.y*1.5f;
          if (entity->position.y >= limit) {
            entity->position.y = -entity->target_position.y;
            entity->shoot_cooldown = entity->shoot_time;
            entity->behavior = ENEMY_BEHAVIOR_FLYING;
          }

          countdown(&entity->shoot_cooldown, time);
          if (entity->shoot_cooldown <= 0.0f) {
            Entity *bullet = entity_alloc(state->bullet_manager);
            bullet->is_alive = true;
            bullet->is_player_friendly = false;
            bullet->speed = random_range(&state->entropy, 20.0f, 30.0f);
            bullet->position = entity->position;
            bullet->velocity.x = entity->shoot_direction.x;
            bullet->velocity.y = random_range(&state->entropy, -0.2f, 0.2f);
            bullet->size = make_vector2(2.0f, 2.0f);
            bullet->sprite_index = 81;
            bullet->sprite_w = 1;
            bullet->sprite_h = 1;
            entity->shoot_cooldown = entity->shoot_time;
          }
        } break;
        case ENEMY_ASSASSIN: {
          linear_move(entity, time);

          if (entity->bullet_fired < entity->bullet_per_attack) {
            countdown(&entity->shoot_cooldown, time);
            if (entity->shoot_cooldown <= 0.0f) {
              Entity *bullet = entity_alloc(state->bullet_manager);
              bullet->is_alive = true;
              bullet->is_player_friendly = false;
              bullet->speed = 110.0f;
              bullet->position = entity->position;
              bullet->velocity = make_vector2(0.0f, 1.0f);
              bullet->size = make_vector2(2.0f, 2.0f);
              bullet->sprite_index = 83;
              bullet->sprite_w = 1;
              bullet->sprite_h = 1;

              entity->shoot_cooldown = entity->shoot_time;
              entity->bullet_fired += 1;
            }
          } else {
            entity->velocity.x *= -1.0f;
            entity->bullet_fired = 0;
            entity->charge_countdown = entity->charge_time;
            entity->behavior = ENEMY_BEHAVIOR_IDLE;
          }
        } break;

        case ENEMY_MINE: {
          entity->attack_time = 0.3f;
          countdown(&entity->attack_countdown, time);
          if (entity->attack_countdown <= 0.0f) {
            entity->flash_countdown = entity->flash_time;
            entity->attack_count += 1;
            entity->attack_countdown = entity->attack_time;
          }
          if (entity->attack_count > 3) {
            hit_enemy(state, entity, entity->hp);
            for (u32 i = 0; i < 16; ++i) {
              Entity *bullet = entity_alloc(state->bullet_manager);
              bullet->is_alive = true;
              bullet->is_player_friendly = false;
              bullet->speed = 60.0f;
              bullet->position = entity->position;
              f32 theta_radians = radians_pi32(i*22.5f);
              bullet->velocity.x = cos_f32(theta_radians);
              bullet->velocity.y = sin_f32(theta_radians);
              bullet->size = make_vector2(2.0f, 2.0f);
              bullet->sprite_index = 112;
              bullet->sprite_w = 1;
              bullet->sprite_h = 1;
            }
          }
        } break;
      }
    } break;
    default: {
      invalid_path;
    } break;
  }
}

// TODO: TEMP
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

///////////////////////
// NOTE: Wave Functions

internal void
reset_enemy_attack_time(Game_State *state) {
  state->enemy_attack_time = 0.0f;
  state->enemy_attack_cooldown = random_range(&state->entropy, 0.2f, 0.4f);
}

internal void
wave_reset(Game_State *state) {
  state->wave_warn_time = state->time + state->wave_warn_cooldown;
  state->enemy_spawn_time = state->wave_warn_time + state->enemy_spawn_cooldown;
  reset_enemy_attack_time(state);
  state->enemies_spawned = false;
  state->wave_enemy_count = 0;
  state->wave_remaining_enemy_count = 0;
}

internal void
wave_init(Game_State *state) {
  state->wave_index = 0;
  state->wave_warn_cooldown = 2.5f;
  state->enemy_spawn_cooldown = 0.4f;
  wave_reset(state);
}

internal void
wave_next(Game_State *state) {
  wave_reset(state);
  state->wave_index += 1;
  if (state->wave_index == WAVE_MAX) {
    state->player_win_position_t = 0.0f;
    change_screen(state, SCREEN_WIN);
  }
}

///////////////////////////////

internal void
change_screen(Game_State *state, Screen_State screen) {
  state->screen_state = screen;
  switch (screen) {
    case SCREEN_GAME: {
      state->record_time = 0.0f;
      state->next_explosion_particle = 0;
      state->next_flash_particle = 0;
      mem_zero_array(state->explosion_particles);
      mem_zero_array(state->flash_particles);
      player_init(state);
      wave_init(state);
    } break;
    case SCREEN_MENU:
    case SCREEN_WIN:
    case SCREEN_OVER: {
      remove_all_entities(state->enemy_manager);
      remove_all_entities(state->bullet_manager);
      state->wave_warn_time = 0.0f;
      state->lock_input_time = state->lock_input_cooldown + state->time;
    } break;
  }
}

shared_function
KRUEGER_CONFIG_PROC(krueger_config) {
  u32 window_scale = 5;
  config->render_w = 128;
  config->render_h = 128;
  config->window_w = window_scale*config->render_w;
  config->window_h = window_scale*config->render_h;
  config->window_title = str8_lit("STARFIGHTER");
}

shared_function
KRUEGER_INIT_PROC(krueger_init) {
  thread_context_select(thread_context);

  assert(sizeof(Game_State) <= memory->size);
  Game_State *state = (Game_State *)memory->ptr;

  Date_Time date_time = platform_get_date_time();
  Dense_Time dense_time = dense_time_from_date_time(date_time);
  state->entropy = random_seed((u32)dense_time);

  uxx half_memory_size = memory->size/2;
  uxx perm_memory_size = half_memory_size - sizeof(Game_State);
  uxx temp_memory_size = half_memory_size;

  u8 *perm_memory_ptr = (u8 *)memory->ptr + sizeof(Game_State);
  u8 *temp_memory_ptr = memory->ptr + half_memory_size;

  state->main_arena = arena_alloc(.base = perm_memory_ptr, .res_size = perm_memory_size);
  state->temp_arena = arena_alloc(.base = temp_memory_ptr, .res_size = temp_memory_size);
  
  state->font_image = load_bmp(state->main_arena, str8_lit("../res/pico8_font.bmp"));
  state->sprites_image = load_bmp(state->main_arena, str8_lit("../res/sprites.bmp"));

  state->font_tilemap = make_tilemap(state->font_image, 4, 6, 16, 6);
  state->sprites_tilemap = make_tilemap(state->sprites_image, 8, 8, 16, 16);

  String8 chars = str8_lit(
    " !\"#$%&'()*+,-./"
    "0123456789:;<=>?"
    "@ABCDEFGHIJKLMNO"
    "PQRSTUVWXYZ[\\]^_"
    "`abcdefghijklmno"
    "pqrstuvwxyz{|}~ "
  );

  chars = str8_copy(state->main_arena, chars);
  state->font = make_font(chars, state->font_tilemap);
  state->draw_buffer = image_alloc(config.render_w, config.render_h);

  state->screen_state = SCREEN_MENU;
  state->pause = false;
  state->time = 0.0f;
  state->shake_oscillation = 0.0f;
  state->blink_frequency = 12.0f;
  state->lock_input_cooldown = 1.0f;
  state->lock_input_time = state->lock_input_cooldown;

  state->player = push_array(state->main_arena, Entity, 1);
  state->enemy_manager = entity_manager_alloc(state->main_arena, 256);
  state->bullet_manager = entity_manager_alloc(state->main_arena, 1024);

  f32 min_star_speed = 32.0f;
  f32 max_star_speed = 64.0f;

  for (u32 particle_index = 0;
       particle_index < array_count(state->star_particles);
       ++particle_index) {
    Particle *particle = state->star_particles + particle_index;
    particle->radius = (f32)random_choice(&state->entropy, 3);
    particle->speed = random_range(&state->entropy, min_star_speed, max_star_speed);

    if (particle->speed >= 0.5f*(min_star_speed + max_star_speed)) {
      particle->color = CP_DARK_GRAY;
    } else {
      particle->color = CP_DARK_BLUE;
    }

    particle->position.x = random_unilateral(&state->entropy)*config.render_w;
    particle->position.y = random_unilateral(&state->entropy)*config.render_h;
    particle->velocity = make_vector2(0.0f, -1.0f);
  }

  state->draw_time_info = false;
  state->draw_debug_info = false;
}

shared_function
KRUEGER_FRAME_PROC(krueger_frame) {
  thread_context_select(thread_context);

  Game_State *state = (Game_State *)memory->ptr;
  state->time += time->dt_sec;

  Digital_Button *kbd = input->kbd;
  if (kbd[KEY_F1].pressed) state->draw_debug_info = !state->draw_debug_info;
  if (kbd[KEY_F2].pressed) state->draw_time_info = !state->draw_time_info;
  
  b32 quit = false;
  if (kbd[KEY_P].pressed) state->pause = !state->pause;
  if (state->pause) return(quit);

  Image draw_buffer = state->draw_buffer;
  image_fill(draw_buffer, CP_BLACK);
 
#if 0
  s32 tile_count_x = (s32)(draw_buffer.width/state->tile_size);
  s32 tile_count_y = (s32)(draw_buffer.height/state->tile_size);

  for (s32 y = 0; y < tile_count_y; ++y) {
    for (s32 x = 0; x < tile_count_x; ++x) {
      s32 x0 = (s32)(x*state->tile_size);
      s32 y0 = (s32)(y*state->tile_size);
      s32 x1 = (s32)(x0 + state->tile_size);
      s32 y1 = (s32)(y0 + state->tile_size);
      u32 color = CP_DARK_GRAY;
      if ((x + y) % 2 == 0) {
        color = CP_DARK_BLUE;
      }
      draw_rect_fill(draw_buffer, x0, y0, x1, y1, color);
    }
  }
#endif

  // NOTE: star particles
  for (u32 particle_index = 0;
       particle_index < array_count(state->star_particles);
       ++particle_index) {
    Particle *particle = state->star_particles + particle_index;
    if (state->time < state->wave_warn_time ||
      state->screen_state == SCREEN_WIN) {
      particle->position.y += 2.5f*particle->speed*time->dt_sec;
    } else {
      particle->position.y += particle->speed*time->dt_sec;
    }
    if (particle->position.y - particle->radius > (f32)draw_buffer.height) {
      particle->position.x = random_unilateral(&state->entropy)*draw_buffer.width;
      particle->position.y = -particle->radius;
    }

    if (particle->radius <= 1.0f) {
      draw_circle_line2(draw_buffer, particle->position,
                        particle->radius, particle->color);
    } else {
      draw_circle_fill2(draw_buffer, particle->position,
                        particle->radius, particle->color);
    }
  }

  // NOTE: explosion particles
  for (u32 particle_index = 0;
       particle_index < array_count(state->explosion_particles);
       ++particle_index) {
    Particle *particle = state->explosion_particles + particle_index;
    if (particle->radius > 0.0f) {
      Vector2 velocity = vector2_mul(particle->velocity, particle->speed*time->dt_sec);
      particle->position = vector2_add(particle->position, velocity);

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

      draw_circle_fill2(draw_buffer, particle->position,
                        particle->radius, particle->color);
    }
  }

  // NOTE: flash particles
  for (u32 particle_index = 0;
       particle_index < array_count(state->flash_particles);
       ++particle_index) {
    Particle *particle = state->flash_particles + particle_index;
    if (particle->life > 0.0f) {
      particle->life += particle->dlife*time->dt_sec;
      particle->time += time->dt_sec;
      if (sin_f32(particle->time*48.0f) > 0.5f) {
        image_fill(draw_buffer, particle->color);
      }
    }
  }

  // NOTE: state
  switch (state->screen_state) {
    case SCREEN_MENU: {
      if (kbd[KEY_Q].pressed) quit = true;

      s32 bw = draw_buffer.width;
      s32 bh = draw_buffer.height;

      { // NOTE: draw title
        draw_text_align(draw_buffer, state->font, str8_lit("STARFIGHTER"),
                        vec2(bw/2.0f, bh/3.0f), vec2(0.5f, 0.5f), CP_RED);
      }
      
      if (state->time > state->lock_input_time) {
        if (kbd[KEY_Z].pressed) change_screen(state, SCREEN_GAME);

        String8 text = str8_lit("press [z] to start");
        Vector2 position = make_vector2(bw/2.0f, bh - bh/3.0f);

        if (sin_f32(state->time*state->blink_frequency) > -0.5f) {
          draw_text_align(draw_buffer, state->font, text,
                          position, vec2(0.5f, 0.5f), CP_DARK_BROWN);
        } else {
          draw_text_align(draw_buffer, state->font, text,
                          position, vec2(0.5f, 0.5f), CP_GRAY);
        }
      }
    } break;

    case SCREEN_GAME: {
      if (kbd[KEY_Q].pressed) change_screen(state, SCREEN_MENU);
      state->record_time += time->dt_sec;

      { // NOTE: update player
        Entity *player = state->player;
        if (player->is_alive) {
          countdown(&player->invencible_countdown, time);
          if (player->invencible_countdown <= 0.0f) {
            player_emit_flame_particles(player, state, time);
          }

          player_update_and_draw_particles(player, time, draw_buffer);
          update_entity_flame(time, player);

          player->velocity = make_vector2(0.0f, 0.0f);
          player->sprite_index = 1;

          if (kbd[KEY_UP].is_down) {
            player->velocity.y = -1.0f;
          }
          if (kbd[KEY_LEFT].is_down) {
            player->velocity.x = -1.0f;
            player->sprite_index = 0;
          }
          if (kbd[KEY_DOWN].is_down) {
            player->velocity.y = 1.0f;
          }
          if (kbd[KEY_RIGHT].is_down) {
            player->velocity.x = 1.0f;
            player->sprite_index = 2;
          }
          if (player->velocity.x != 0.0f && player->velocity.y != 0.0f) {
            player->velocity = vector2_mul(player->velocity, 0.707106781187f);
          }

          linear_move(player, time);

          if (player->position.x < 0.0f) {
            player->position.x = 0;
          }
          if (player->position.y < 0.0f) {
            player->position.y = 0;
          }
          if (player->position.x > (f32)draw_buffer.width) {
            player->position.x = (f32)draw_buffer.width;
          }
          if (player->position.y > (f32)draw_buffer.height) {
            player->position.y = (f32)draw_buffer.height;
          }

          // NOTE: player fire bullets
          countdown(&player->shoot_cooldown, time);
          if (kbd[KEY_Z].is_down) {
            if (player->shoot_cooldown <= 0.0f) {
              Entity *bullet = entity_alloc(state->bullet_manager);
              bullet->is_alive = true;
              bullet->is_player_friendly = true;
              bullet->speed = 390.0f;
              bullet->velocity = make_vector2(0.0f, -1.0f);
              bullet->position = player->position;
              bullet->position.x += (state->player_shoot_side++ % 2 == 0) ? 1.0f : -1.0f;
              bullet->size = make_vector2(4.0f, 4.0f);
              bullet->sprite_index = 4;
              bullet->sprite_w = 1;
              bullet->sprite_h = 1;
              player->shoot_cooldown = player->shoot_time;
              state->player_shoot_r = state->player_max_shoot_r;
            }
          }
        }

        // NOTE: player fire bomb
        if (kbd[KEY_X].is_down &&
            state->bomb_count > 0 &&
            player->invencible_countdown <= 0.0f) {
          state->bomb_count -= 1;
          player->invencible_countdown = player->invencible_time*0.7f;
          state->shake_oscillation += 8.0f;
          emit_flash(state, CP_ORANGE);
          for (Entity *bullet = state->bullet_manager->list.first;
               bullet != 0;
               bullet = bullet->next) {
            if (bullet->is_alive &&
                !bullet->is_player_friendly) {
              bullet->is_alive = false;
              emit_explosion(state, bullet->position);
            }
          }
        }
      }

#if BUILD_DEBUG
      // NOTE: kill all enemies [debug code only]
      if (state->enemies_spawned) {
        if (kbd[KEY_K].pressed) {
          remove_all_entities(state->bullet_manager);
          for (Entity *enemy = state->enemy_manager->list.first;
               enemy != 0;
               enemy = enemy->next) {
            hit_enemy(state, enemy, enemy->hp);
          }
        }
      }
#endif

      // NOTE: go to next wave when all enemies died
      if (state->enemies_spawned &&
          state->wave_remaining_enemy_count == 0) {
        wave_next(state);
      }

      // NOTE: add enemies
      if (state->time > state->wave_warn_time &&
          state->time > state->enemy_spawn_time &&
          !state->enemies_spawned) {
        state->enemies_spawned = true;
        u32 *wave = waves[state->wave_index];
        for (u32 y = 0; y < WAVE_H; ++y) {
          for (u32 x = 0; x < WAVE_W; ++x) {
            Enemy_Type entity_type = wave[y*WAVE_W + x];
            if (entity_type != ENEMY_NULL) {
              Entity *enemy = add_enemy(state, entity_type);

              f32 gap_x = enemy->size.x*3.0f;
              f32 gap_y = enemy->size.y*1.5f;

              f32 offset_x = 0.5f*(draw_buffer.width - WAVE_W*gap_x + gap_x);
              f32 offset_y = -1.0f*(WAVE_H*gap_y);

              enemy->position.x = offset_x + x*gap_x;
              enemy->position.y = offset_y + y*gap_y;

              gap_x = enemy->size.x*1.5f;
              offset_x = 0.5f*(draw_buffer.width - WAVE_W*gap_x + gap_x);
              offset_y = TILE_SIZE*2.0f;

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
          if (state->enemy_manager->list.count > 0) {
            u32 enemy_index = random_choice(&state->entropy,
                                            state->enemy_manager->list.count);
            Entity *enemy = state->enemy_manager->list.first;
            for (u32 j = 0; j < enemy_index; ++j) {
              enemy = enemy->next;
            }
            if (enemy->is_alive) {
              if (enemy->type != ENEMY_MINE) {
                if (enemy->behavior == ENEMY_BEHAVIOR_IDLE) {
                  enemy->behavior = ENEMY_BEHAVIOR_CHARGE;
                }
              }
            }
            reset_enemy_attack_time(state);
          }
        }
      }

      // NOTE: simulate enemies
      for (Entity *enemy = state->enemy_manager->list.first;
           enemy != 0;
           enemy = enemy->next) {
        if (enemy->is_alive) {
          countdown(&enemy->flash_countdown, time);
          enemy_behave(enemy, state, time);
          enemy_emit_flame_particles(enemy, state, time);
          update_entity_flame(time, enemy);
        }
      }

      // NOTE: simulate bullets
      for (Entity *bullet = state->bullet_manager->list.first;
           bullet != 0;
           bullet = bullet->next) {
        if (bullet->is_alive) {
          linear_move(bullet, time);

          f32 half_size = TILE_SIZE*0.5f;

          f32 x = bullet->position.x;
          f32 y = bullet->position.y;

          Vector2 min = make_vector2(0.0f, 0.0f);
          Vector2 max = make_vector2((f32)state->draw_buffer.width,
                                     (f32)state->draw_buffer.height);

          if (((x + half_size) < min.x) || ((x - half_size) > max.x) ||
              ((y + half_size) < min.y) || ((y - half_size) > max.y)) {
            bullet->is_alive = false;
          }
        }
      }

      { // NOTE: player x enemy collision
        Entity *player = state->player;
        if (player->is_alive) {
          for (Entity *enemy = state->enemy_manager->list.first;
               enemy != 0;
               enemy = enemy->next) {
            if (enemy->is_alive) {
              if (entity_collide(*enemy, *player)) {
                if (hit_player(state, player, 1)) {
                  if (enemy->type != ENEMY_BOSS) {
                    hit_enemy(state, enemy, enemy->hp);
                  }
                }
              }
            }
          }
        }
      }

      // NOTE: bullet collision
      for (Entity *bullet = state->bullet_manager->list.first;
           bullet != 0;
           bullet = bullet->next) {
        if (bullet->is_alive) {
          if (!bullet->is_player_friendly) {
            Entity *player = state->player;
            if (player->is_alive) {
              if (entity_collide(*bullet, *player)) {
                if (hit_player(state, player, 1)) {
                  bullet->is_alive = false;
                }
              }
            }
          } else {
            for (Entity *enemy = state->enemy_manager->list.first;
                 enemy != 0;
                 enemy = enemy->next) {
              if (enemy->is_alive) {
                if (entity_collide(*bullet, *enemy)) {
                  hit_enemy(state, enemy, 1);
                  bullet->is_alive = false;
                }
              }
            }
          }
        }
      }

      remove_dead_entities(state->bullet_manager);
      remove_dead_entities(state->enemy_manager);

      // TODO: get rid off enemy_update_draw_particles;
      draw_bullets(draw_buffer, state->bullet_manager, state->sprites_tilemap);
      draw_enemies(draw_buffer, state->enemy_manager, state->sprites_tilemap, time);

      { // NOTE: draw player
        Entity *player = state->player;
        if (player->is_alive) {
          { // NOTE: draw player shoot effect
            if (state->player_shoot_r > 0.0f) {
              state->player_shoot_r += state->player_dshoot_r*time->dt_sec;
            }
            if (state->player_shoot_r < 0.0f) state->player_shoot_r = 0.0f;

            Vector2 position = player->position;
            f32 gap = 1.0f;
            position.x += (state->player_shoot_side%2 == 0) ? -gap : gap;
            position.y -= 3.0f;
            draw_circle_fill2(draw_buffer, position, state->player_shoot_r, CP_WHITE);
          }

          if (player->invencible_countdown <= 0.0f) {
            draw_sprite(draw_buffer, state->sprites_tilemap,
                        player->next_flame_sprite_frame, 1, 1,
                        player->position, vec2(0.5f, 0.5f), vec2(0.0f, 8.0f));
            draw_sprite(draw_buffer, state->sprites_tilemap, 
                        player->sprite_index, player->sprite_w, player->sprite_h,
                        player->position, vec2(0.5f, 0.5f), vec2(0.0f, 0.0f));
          } else {
            if (sin_f32(thousand(1.0f)*state->time) <= 0.0f) {
              draw_sprite(draw_buffer, state->sprites_tilemap,
                          player->sprite_index, player->sprite_w, player->sprite_h,
                          player->position, vec2(0.5f, 0.5f), vec2(0.0f, 0.0f));
            }
          }
        }
      }

      // NOTE: draw ui
      s32 bw = draw_buffer.width;
      s32 bh = draw_buffer.height;
      
      if (state->wave_index != LAST_WAVE) {
        { // NOTE: draw enemy count
          Temp scratch = scratch_begin(0, 0);
          String8 text = push_str8_fmt(scratch.arena, "%d/%d",
                                       state->wave_remaining_enemy_count,
                                       state->wave_enemy_count);
          Vector2 position = make_vector2(bw/2.0f, 1.0f);
          draw_text_align(draw_buffer, state->font, text,
                          position, vec2(0.5f, 0.0f), CP_RED);
          scratch_end(scratch);
        }
      }

      // NOTE: draw player hp
      for (u32 hp_index = 0;
           hp_index < state->player->hp;
           ++hp_index) {
        u32 sprite_index = 32;
        Image sprite = tilemap_get_tile(state->sprites_tilemap, sprite_index);
        Vector2 position = make_vector2(0.0f, 0.0f);
        Vector2 offset = make_vector2((f32)hp_index*sprite.width, 0.0f);
        draw_sprite(draw_buffer, state->sprites_tilemap,
                    sprite_index, 1, 1,
                    position, vec2(0.0f, 0.0f), offset);
      }

      // NOTE: draw bomb
      for (u32 bomb_index = 0;
          bomb_index < state->bomb_count;
          ++bomb_index) {
        u32 sprite_index = 34;
        Image sprite = tilemap_get_tile(state->sprites_tilemap, sprite_index);
        Vector2 position = make_vector2((f32)(bw - bomb_index*sprite.width), 0.0f);
        draw_sprite(draw_buffer, state->sprites_tilemap,
                    sprite_index, 1, 1,
                    position, vec2(1.0f, 0.0f), vec2(0.0f, 0.0f));
      }

      // NOTE: draw wave warn
      if (state->time < state->wave_warn_time) {
        Temp scratch = scratch_begin(0, 0);
        String8 text = push_str8_fmt(scratch.arena, "WAVE %d", state->wave_index + 1);
        Vector2 position = make_vector2(bw/2.0f, bh/5.0f);
        if (sin_f32(state->time*state->blink_frequency) > -0.5f) {
          draw_text_align(draw_buffer, state->font, text,
                          position, vec2(0.5f, 0.5f), CP_DARK_RED);
        } else {
          draw_text_align(draw_buffer, state->font, text,
                          position, vec2(0.5f, 0.5f), CP_RED);
        }
        scratch_end(scratch);
      }

      if (state->wave_index == LAST_WAVE) {
        Entity *boss = state->enemy_manager->list.first;
        if (boss) {
          f32 margin = TILE_SIZE;
          f32 top_margin = TILE_SIZE*1.25f;

          f32 bar_w = draw_buffer.width - 2.0f*margin;
          f32 bar_h = TILE_SIZE*0.5f;

          s32 x0 = (s32)margin;
          s32 y0 = (s32)top_margin;
          s32 x1 = (s32)(x0 + bar_w);
          s32 y1 = (s32)(y0 + bar_h);

          draw_rect_fill(draw_buffer, x0, y0, x1, y1, CP_BLACK);

          x0 += 1;
          y0 += 1;
          x1 -= 1;
          y1 -= 1;
          bar_w -= 2;
          bar_h -= 2;

          draw_rect_fill(draw_buffer, x0, y0, x1, y1, CP_DARK_BLUE);

          f32 t = (f32)boss->hp/(f32)boss->max_hp;
          x1 = (s32)(x0 + bar_w*t);

          draw_rect_fill(draw_buffer, x0, y0, x1, y1, CP_RED);
        }
      }

    } break;

    case SCREEN_OVER: {
      if (kbd[KEY_Q].pressed) quit = true;

      s32 bw = draw_buffer.width;
      s32 bh = draw_buffer.height;

      { // NOTE: draw game over
        Temp scratch = scratch_begin(0, 0);
        String8 text = push_str8_lit(scratch.arena, "GAME OVER");
        Vector2 position = make_vector2(bw/2.0f, bh/3.0f);
        draw_text_align(draw_buffer, state->font, text,
                        position, vec2(0.5f, 0.5f), CP_RED);
        scratch_end(scratch);
      }
      
      if (state->time > state->lock_input_time - state->lock_input_cooldown*0.5f) {
        { // NOTE: draw record
          Temp scratch = scratch_begin(0, 0);
          String8 text = push_str8_fmt(scratch.arena, "RECORD: %.2f", state->record_time);
          Vector2 position = make_vector2(bw/2.0f, bh/2.0f);
          draw_text_align(draw_buffer, state->font, text,
                          position, vec2(0.5f, 0.5f), CP_WHITE);
          scratch_end(scratch);
        }
      }

      if (state->time > state->lock_input_time) {
        if (kbd[KEY_Z].pressed) change_screen(state, SCREEN_MENU);
        Temp scratch = scratch_begin(0, 0);

        String8 text0 = push_str8_lit(scratch.arena, "press [z] to go");
        String8 text1 = push_str8_lit(scratch.arena, "back to menu");

        s32 th = state->font.tilemap.tile_h;

        Vector2 position = make_vector2(bw/2.0f, bh + bh/3.0f);

        if (sin_f32(state->time*state->blink_frequency) > -0.5f) {
          draw_text_align(draw_buffer, state->font, text0,
                          position, vec2(0.5f, 0.5f), CP_DARK_BROWN);
          position.y += th;
          draw_text_align(draw_buffer, state->font, text1,
                          position, vec2(0.5f, 0.5f), CP_DARK_BROWN);
        } else {
          draw_text_align(draw_buffer, state->font, text0,
                          position, vec2(0.5f, 0.5f), CP_GRAY);
          position.y += th;
          draw_text_align(draw_buffer, state->font, text1,
                          position, vec2(0.5f, 0.5f), CP_GRAY);
        }
        scratch_end(scratch);
      }
    } break;

    case SCREEN_WIN: {
      if (kbd[KEY_Q].pressed) quit = true;

      s32 bw = draw_buffer.width;
      s32 bh = draw_buffer.height;

      s32 th = state->font.tilemap.tile_h;

      // NOTE: draw player animation
      state->player_win_position_t += time->dt_sec*0.02f;
      if (state->player_win_position_t >= 1.0f) {
        state->player_win_position_t = 1.0f;
      }

      f32 t = state->player_win_position_t;
      
      Entity *player = state->player;

      Vector2 position0 = player->position;
      Vector2 position1 = vector2_mul(vec2((f32)bw, (f32)bh), 0.5f);

      player->position = vector2_lerp(position0, position1, t);

      player_emit_flame_particles(player, state, time);
      player_update_and_draw_particles(player, time, draw_buffer);
      update_entity_flame(time, player);

      draw_sprite(draw_buffer, state->sprites_tilemap,
                  player->next_flame_sprite_frame, 1, 1,
                  player->position, vec2(0.5f, 0.5f), vec2(0.0f, 8.0f));
      draw_sprite(draw_buffer, state->sprites_tilemap, 1, 1, 1,
                  player->position, vec2(0.5f, 0.5f), vec2(0.0f, 0.0f));

      { // NOTE: draw win
        Temp scratch = scratch_begin(0, 0);
        String8 text = push_str8_lit(scratch.arena, "YOU WIN!");
        Vector2 position = make_vector2(bw/2.0f, bh/4.0f + th/2.0f);
        draw_text_align(draw_buffer, state->font, text,
                        position, vec2(0.5f, 0.5f), CP_BLUE);
        scratch_end(scratch);
      }

      if (state->time > state->lock_input_time - state->lock_input_cooldown*0.5f) {
        { // NOTE: draw record
          Temp scratch = scratch_begin(0, 0);
          String8 text = push_str8_fmt(scratch.arena, "RECORD: %.2f", state->record_time);
          Vector2 position = make_vector2(bw/2.0f, bh/3.0f + th/2.0f);
          draw_text_align(draw_buffer, state->font, text,
                          position, vec2(0.5f, 0.5f), CP_WHITE);
          scratch_end(scratch);
        }
      }

      if (state->time > state->lock_input_time) {
        if (kbd[KEY_Z].pressed) change_screen(state, SCREEN_MENU);
        Temp scratch = scratch_begin(0, 0);

        String8 text0 = push_str8_lit(scratch.arena, "press [z] to go");
        String8 text1 = push_str8_lit(scratch.arena, "back to menu");

        Vector2 position = make_vector2(bw/2.0f, bh - bh/3.0f);

        if (sin_f32(state->time*state->blink_frequency) > -0.5f) {
          draw_text_align(draw_buffer, state->font, text0,
                          position, vec2(0.5f, 0.5f), CP_DARK_BROWN);
          position.y += th;
          draw_text_align(draw_buffer, state->font, text1,
                          position, vec2(0.5f, 0.5f), CP_DARK_BROWN);
        } else {
          draw_text_align(draw_buffer, state->font, text0,
                          position, vec2(0.5f, 0.5f), CP_GRAY);
          position.y += th;
          draw_text_align(draw_buffer, state->font, text1,
                          position, vec2(0.5f, 0.5f), CP_GRAY);
        }
        scratch_end(scratch);
      }
    } break;
  }

  // NOTE: draw time info
  if (state->draw_time_info) {
    Temp scratch = scratch_begin(0, 0);

    f32 fps = million(1.0f)/time->_dt_us;
    f32 ms = time->_dt_us/thousand(1.0f);
    
    String8_List *list = &state->time_string_list;
    mem_zero_struct(list);

    str8_list_push_fmt(scratch.arena, list, "%.2f fps", fps);
    str8_list_push_fmt(scratch.arena, list, "%.2f ms ", ms);
    
    s32 bw = draw_buffer.width;
    s32 bh = draw_buffer.height;

    s32 tw = state->font.tilemap.tile_w;
    s32 th = state->font.tilemap.tile_h;

    Vector2 position = make_vector2(bw - 2.0f, bh - list->count*th - 2.0f);
    
    for (String8_Node *node = list->first; node != 0; node = node->next) {
      String8 string = node->string;
      Vector2 min = vector2_sub(position, vec2((f32)string.len*tw, 0.0f));
      Vector2 max = make_vector2(position.x + 1, min.y + th + 1.0f);
      draw_rect_fill2(draw_buffer, min, max, CP_BLACK);
      draw_text_align(draw_buffer, state->font, string,
                      position, vec2(1.0f, 0.0f), CP_DARK_GRAY);
      position.y += th;
    }

    scratch_end(scratch);
  }
  
  // NOTE: draw debug info
  if (state->draw_debug_info) {
    Temp scratch = scratch_begin(0, 0);

    String8_List *list = &state->debug_string_list;
    mem_zero_struct(list);

    Entity_Manager *manager = state->enemy_manager;
    u32 count = manager->list.count;
    u32 total_count = manager->list.total_count;
    str8_list_push_fmt(scratch.arena, list, "enemies:%d/%d", count, total_count);
    
    manager = state->bullet_manager;
    count = manager->list.count;
    total_count = manager->list.total_count;
    str8_list_push_fmt(scratch.arena, list, "bullets:%d/%d", count, total_count);

    // s32 bw = draw_buffer.width;
    s32 bh = draw_buffer.height;

    s32 tw = state->font.tilemap.tile_w;
    s32 th = state->font.tilemap.tile_h;

    Vector2 position = make_vector2(1.0f, bh - list->count*th - 2.0f);

    for (String8_Node *node = list->first; node != 0; node = node->next) {
      String8 string = node->string;
      Vector2 min = position;
      Vector2 max = vector2_add(min, vec2(string.len*tw + 1.0f, th + 1.0f));
      draw_rect_fill2(draw_buffer, min, max, CP_BLACK);
      draw_text_align(draw_buffer, state->font, string,
                      position, vec2(0.0f, 0.0f), CP_DARK_GRAY);
      position.y += th;
    }

    scratch_end(scratch);
  }
 
  // NOTE: shake screen
  f32 x = random_bilateral(&state->entropy)*state->shake_oscillation;
  f32 y = random_bilateral(&state->entropy)*state->shake_oscillation;

  if (state->shake_oscillation > 10.0f) {
    state->shake_oscillation *= 0.9f;
  }

  state->shake_oscillation -= 2.0f*state->shake_oscillation*time->dt_sec;
  if (state->shake_oscillation < 0.0f) {
    state->shake_oscillation = 0.0f;
  }

  image_fill(*back_buffer, CP_BLACK);
  draw_texture_f32(*back_buffer, draw_buffer, x, y);

  return(quit);
}
