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
    .pitch = image.width,
    .pixels = image.pixels + (y*image.pitch + x),
  };
  return(result);
}

internal void
image_fill(Image image, u32 color) {
  for (u32 y = 0; y < image.height; ++y)  {
    for (u32 x = 0; x < image.width; ++x) {
      image.pixels[y*image.pitch + x] = color;
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
  f32 dt;
  f32 dt_us;
  f32 dt_ms;
  f32 dt_sec;
  f32 sec;
} Clock;

typedef struct {
  uxx permanent_memory_size;
  uxx transient_memory_size;
  u8 *permanent_memory_ptr;
  u8 *transient_memory_ptr;
} Memory;

#define KRUEGER_INIT_PROC(x) void x(Memory *memory)
#define KRUEGER_FRAME_PROC(x) void x(Memory *memory, Image *back_buffer, Input *input, Clock *time)

typedef KRUEGER_INIT_PROC(krueger_init_proc);
typedef KRUEGER_FRAME_PROC(krueger_frame_proc);

#define KRUEGER_PROC_LIST \
  PROC(krueger_init) \
  PROC(krueger_frame)

#endif // KRUEGER_SHARED_H
