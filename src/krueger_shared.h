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

#define UPDATE_AND_RENDER_PROC(x) void x(Image back_buffer, Input input, Clock time)
typedef UPDATE_AND_RENDER_PROC(update_and_render_proc);

#define SHARED_PROC_LIST \
  PROC(update_and_render)

#endif // KRUEGER_SHARED_H
