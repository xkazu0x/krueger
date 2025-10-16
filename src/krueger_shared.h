#ifndef KRUEGER_SHARED_H
#define KRUEGER_SHARED_H

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

internal Image
image_alloc(s32 w, s32 h) {
  uxx buf_size = w*h*sizeof(u32);
  u32 *buf = platform_reserve(buf_size);
  platform_commit(buf, buf_size);
  Image result = {
    .width = w,
    .height = h,
    .pixels = buf,
  };
  return(result);
}

internal void
image_release(Image image) {
  uxx image_size = image.width*image.height*sizeof(u32);
  platform_release(image.pixels, image_size);
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
