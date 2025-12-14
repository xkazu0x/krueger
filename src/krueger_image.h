#ifndef KRUEGER_IMAGE_H
#define KRUEGER_IMAGE_H

typedef struct {
  u32 width;
  u32 height;
  u32 pitch;
  u32 *pixels;
} Image;

internal Image  make_image(u32 *pixels, u32 width, u32 height);
internal Image  image_alloc(u32 width, u32 height);
internal void   image_release(Image *image);
internal Image  image_scissor(Image image, u32 x, u32 y, u32 w, u32 h);
internal void   image_fill(Image dst, u32 color);

#endif // KRUEGER_IMAGE_H
