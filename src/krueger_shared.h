#ifndef KRUEGER_SHARED_H
#define KRUEGER_SHARED_H

typedef struct {
  u32 width;
  u32 height;
  u32 pitch;
  u32 *pixels;
} Image;

internal Image
make_image(u32 *pixels, u32 w, u32 h) {
  Image result = {
    .width = w,
    .height = h,
    .pitch = w,
    .pixels = pixels,
  };
  return(result);
}

internal Image
make_subimage(Image image, 
              u32 x, u32 y,
              u32 w, u32 h) {
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
  uxx memory_size;
  u8 *memory_ptr;
} Memory;

#define KRUEGER_INIT_PROC(x) void x(Memory *memory, Image *back_buffer)
#define KRUEGER_FRAME_PROC(x) void x(Memory *memory, Image *back_buffer, Input *input, Clock *time)

typedef KRUEGER_INIT_PROC(krueger_init_proc);
typedef KRUEGER_FRAME_PROC(krueger_frame_proc);

#define KRUEGER_PROC_LIST \
  PROC(krueger_init) \
  PROC(krueger_frame)

#endif // KRUEGER_SHARED_H
