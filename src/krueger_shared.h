#ifndef KRUEGER_SHARED_H
#define KRUEGER_SHARED_H

typedef struct {
  u32 width;
  u32 height;
  u32 pitch;
  u32 *pixels;
} Image;

internal Image
make_image(u32 *pixels, u32 width, u32 height) {
  Image result = {
    .width = width,
    .height = height,
    .pitch = width,
    .pixels = pixels,
  };
  return(result);
}

internal Image
image_alloc(u32 width, u32 height) {
  uxx size = width*height*sizeof(u32);
  u32 *pixels = platform_reserve(size);
  platform_commit(pixels, size);
  Image result = make_image(pixels, width, height);
  return(result);
}

internal void
image_release(Image *image) {
  uxx size = image->width*image->height*sizeof(u32);
  platform_release(image->pixels, size);
  image->width = 0;
  image->height = 0;
  image->pitch = 0;
  image->pixels = 0;
}

internal Image
image_scissor(Image image, u32 x, u32 y, u32 width, u32 height) {
  Image result = {
    .width = width,
    .height = height,
    .pitch = image.pitch,
    .pixels = image.pixels + (y*image.pitch + x),
  };
  return(result);
}

internal void
image_fill(Image dst, u32 color) {
  for (u32 y = 0; y < dst.height; ++y) {
    for (u32 x = 0; x < dst.width; ++x) {
      dst.pixels[y*dst.pitch + x] = color;
    }
  }
}

typedef struct {
  b32 is_down;
  b32 pressed;
  b32 released;
} Digital_Button;

typedef struct {
  Digital_Button kbd[KEY_MAX];
} Input;

typedef struct {
  // NOTE: this is the one that is supposed
  // to be used to calculate physics.
  f32 dt_sec;

  // NOTE: this is the real time per frame
  f32 _dt_ms;
  u64 _dt_us;
} Clock;

typedef struct {
  uxx size;
  u8 *ptr; // NOTE: REQUIRED to be cleared at startup
} Memory;

typedef struct {
  u32 render_w;
  u32 render_h;
  u32 window_w;
  u32 window_h;
  String8 window_title;
} Krueger_Config;

#define KRUEGER_CONFIG_PROC(x) void x(Krueger_Config *config)
#define KRUEGER_INIT_PROC(x) void x(Thread_Context *thread_context, Memory *memory, Krueger_Config config)
#define KRUEGER_FRAME_PROC(x) b32 x(Thread_Context *thread_context, Memory *memory, Image *back_buffer, Input *input, Clock *time)

typedef KRUEGER_CONFIG_PROC(krueger_config_proc);
typedef KRUEGER_INIT_PROC(krueger_init_proc);
typedef KRUEGER_FRAME_PROC(krueger_frame_proc);

#define KRUEGER_PROC_LIST \
  PROC(krueger_config) \
  PROC(krueger_init) \
  PROC(krueger_frame)

#endif // KRUEGER_SHARED_H
