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

internal Image
make_image(u32 *pixels, u32 width, u32 height) {
  Image image = {
    .width = width,
    .height = height,
    .pixels = pixels,
  };
  return(image);
}

internal void
image_fill(Image image, u32 color) {
  for (uxx i = 0; i < image.width*image.height; ++i) {
    image.pixels[i] = color;
  }
}

internal void
image_copy(Image dst, Image src) {
  for (u32 y = 0; y < dst.height; ++y) {
    for (u32 x = 0; x < dst.width; ++x) {
      u32 nx = x*src.width/dst.width;
      u32 ny = y*src.height/dst.height;
      dst.pixels[y*dst.width + x] = src.pixels[ny*src.width + nx];
    }
  }
}

typedef struct {
  f32 dt_us;
  f32 dt_ms;
  f32 dt_sec;
  f32 us;
  f32 ms;
  f32 sec;
} Clock;

typedef struct Krueger_State Krueger_State;

#define KRUEGER_INIT_PROC(x) Krueger_State *x(void)
typedef KRUEGER_INIT_PROC(krueger_init_proc);

#define KRUEGER_FRAME_PROC(x) void x(Krueger_State *state, Image back_buffer, Input input, Clock time)
typedef KRUEGER_FRAME_PROC(krueger_frame_proc);

#define KRUEGER_PROC_LIST \
  PROC(krueger_init) \
  PROC(krueger_frame)

#endif // KRUEGER_SHARED_H
