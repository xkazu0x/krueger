#include <stdio.h>
#include <stdlib.h>

#define internal static

#include <stdint.h>
#include <stddef.h>
typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef size_t   uxx;
typedef int8_t   s8;
typedef int16_t  s16;
typedef int32_t  s32;
typedef int64_t  s64;
typedef s8       b8;
typedef s16      b16;
typedef s32      b32;
typedef s64      b64;
typedef float    f32;
typedef double   f64;

#pragma pack(push, 1)
typedef struct {
    u16 type;
    u32 size;
    u16 reserved1;
    u16 reserved2;
    u32 offset;
    u32 dib_header_size;
    s32 width_px;
    s32 height_px;
    u16 num_planes;
    u16 bits_per_pixel;
    u32 compression;
    u32 image_size_bytes;
    s32 x_resolution_ppm;
    s32 y_resolution_ppm;
    u32 num_colors;
    u32 important_colors;
} bmp_header;
#pragma pack(pop)

typedef struct {
    u32 width;
    u32 height;
    u32 *pixels;
} image_u32;

internal image_u32
alloc_image(u32 width, u32 height) {
    image_u32 result = {
        .width = width,
        .height = height,
        .pixels = malloc(width*height*sizeof(u32)),
    };
    return(result);
}

internal u32
get_image_size(image_u32 image) {
    u32 result = image.width*image.height*sizeof(u32);
    return(result);
}

internal void
write_image(image_u32 image, char *filename) {
    u32 image_size = get_image_size(image);
    bmp_header header = {
        .type             = 0x4D42,
        .size             = sizeof(bmp_header) + image_size,
        .reserved1        = 0,
        .reserved2        = 0,
        .offset           = sizeof(bmp_header),
        .dib_header_size  = sizeof(bmp_header) - offsetof(bmp_header, dib_header_size),
        .width_px         = image.width,
        .height_px        = image.height,
        .num_planes       = 1,
        .bits_per_pixel   = 32,
        .compression      = 0,
        .image_size_bytes = image_size,
        .x_resolution_ppm = 0,
        .y_resolution_ppm = 0,
        .num_colors       = 0,
        .important_colors = 0,
    };

    FILE *file = fopen(filename, "wb");
    if (file) {
        fwrite(&header, sizeof(header), 1, file);
        fwrite(image.pixels, image_size, 1, file);
        fclose(file);
    } else {
        fprintf(stderr, "[ERROR]: Failed to write file %s\n", filename);
    }
}

int
main(void) {
    image_u32 image = alloc_image(800, 600);
    u32 *out = image.pixels;
    for (u32 y = 0; y < image.height; ++y) {
        for (u32 x = 0; x < image.width; ++x) {
            *out++ = (y < 32) ? 0xFFFF0000 : 0xFF0000FF;
        }
    }
    write_image(image, "output.bmp");
    return(0);
}
