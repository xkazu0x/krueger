#ifndef KRUEGER_IMAGE_C
#define KRUEGER_IMAGE_C

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

#endif // KRUEGER_IMAGE_C
