#ifndef STARFIGHTER_H
#define STARFIGHTER_H

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
image_scissor(Image image, u32 x, u32 y, u32 w, u32 h) {
  Image result = {
    .width = w,
    .height = h,
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
  f32 value;
  b32 is_down;
  b32 pressed;
  b32 released;
} Analog_Button;

typedef union {
  struct { f32 x, y; };
  Vector2 axis;
} Stick;

typedef struct {
#if BUILD_DEBUG
  Digital_Button *keys;
#endif

  Digital_Button confirm;
  Digital_Button pause;
  Digital_Button quit;

  Digital_Button up;
  Digital_Button down;
  Digital_Button left;
  Digital_Button right;

  Digital_Button shoot;
  Digital_Button bomb;

  Stick direction;
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
  b32 is_initialized;
  String8 res_path;
  uxx size;
  u8 *ptr; // NOTE: REQUIRED to be cleared at startup
} Memory;

#define GAME_FRAME_PROC(x)        b32 x(Thread_Context *thread_context, Memory *memory, Image *back_buffer, Input *input, Clock *time)
#define GAME_OUTPUT_SOUND_PROC(x) void x(s16 *samples, u32 num_samples, u32 sample_rate, void *user_data)

typedef GAME_FRAME_PROC(frame_proc);
typedef GAME_OUTPUT_SOUND_PROC(output_sound_proc);

#define GAME_PROC_LIST \
  GAME_PROC(frame) \
  GAME_PROC(output_sound)

#endif // KRUEGER_SHARED_H
