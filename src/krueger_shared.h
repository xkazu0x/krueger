#ifndef KRUEGER_SHARED_H
#define KRUEGER_SHARED_H

typedef u32 Keycode;
enum {
  KEY_NULL,

  KEY_0,
  KEY_1,
  KEY_2,
  KEY_3,
  KEY_4,
  KEY_5,
  KEY_6,
  KEY_7,
  KEY_8,
  KEY_9,

  KEY_A,
  KEY_B,
  KEY_C,
  KEY_D,
  KEY_E,
  KEY_F,
  KEY_G,
  KEY_H,
  KEY_I,
  KEY_J,
  KEY_K,
  KEY_L,
  KEY_M,
  KEY_N,
  KEY_O,
  KEY_P,
  KEY_Q,
  KEY_R,
  KEY_S,
  KEY_T,
  KEY_U,
  KEY_V,
  KEY_W,
  KEY_X,
  KEY_Y,
  KEY_Z,

  KEY_UP,
  KEY_LEFT,
  KEY_DOWN,
  KEY_RIGHT,

  KEY_MAX,
};

typedef struct {
  b32 is_down;
  b32 pressed;
  b32 released;
} Digital_Button;

typedef struct {
  Digital_Button kbd[KEY_MAX];
} Input;

typedef struct {
  u32 width;
  u32 height;
  u32 *pixels;
} Image;

typedef struct {
  f32 dt_sec;
  f32 dt_ms;
  f32 fps;
} Clock;

#define KRUEGER_INIT_PROC(x) void x(Arena *arena)
typedef KRUEGER_INIT_PROC(krueger_init_proc);

#define KRUEGER_FRAME_PROC(x) void x(Arena *arena, Image *back_buffer, Input *input, Clock *time)
typedef KRUEGER_FRAME_PROC(krueger_frame_proc);

#define KRUEGER_PROC_LIST \
  PROC(krueger_init) \
  PROC(krueger_frame)

#endif // KRUEGER_SHARED_H
