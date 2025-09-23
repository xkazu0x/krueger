#include "base.h"

#include <stdio.h>
#include <stdlib.h>

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

typedef struct {
    vec3 color;
} material;

typedef struct {
    vec3 n;
    f32 d;
    u32 mat_id;
} plane;

typedef struct {
    vec3 p;
    f32 r;
    u32 mat_id;
} sphere;

#define array_count(x) (sizeof(x)/sizeof(*(x)))

internal u32
pack_rgba4x8(u8 r, u8 g, u8 b, u8 a) {
    u32 result = ((a << 24) | (r << 16) | (g << 8) | (b << 0));
    return(result);
}

int
main(void) {
    material materials[] = {
        { make_vec3(0.1f, 0.1f, 0.1f) },
        { make_vec3(0.5f, 0.1f, 0.1f) },
    };

    plane planes[] = {
        {
            .n = make_vec3(0.0f, 0.0f, 1.0f),
            .d = 0.0f,
            .mat_id = 1,
        },
    };

    vec3 cam_p = make_vec3(0.0f, -10.0f, 1.0f);
    vec3 cam_z = vec3_normalize(cam_p);
    vec3 cam_x = vec3_normalize(vec3_cross(make_vec3(0.0f, 0.0f, 1.0f), cam_z));
    vec3 cam_y = vec3_normalize(vec3_cross(cam_z, cam_x));

    f32 screen_d = 1.0f;
    f32 screen_w = 1.0f;
    f32 screen_h = 1.0f;
    f32 half_screen_w = 0.5f*screen_w;
    f32 half_screen_h = 0.5f*screen_h;
    vec3 screen_c = vec3_sub(cam_p, vec3_scale(cam_z, screen_d));

    image_u32 image = alloc_image(800, 600);
    u32 *out = image.pixels;
    for (u32 y = 0; y < image.height; ++y) {
        f32 screen_y = -1.0f + 2.0f*((f32)y / (f32)image.height);
        for (u32 x = 0; x < image.width; ++x) {
            f32 screen_x = -1.0f + 2.0f*((f32)x / (f32)image.width);

            vec3 screen_p = vec3_add(screen_c, 
                                     vec3_add(vec3_scale(cam_x, screen_x*half_screen_w), 
                                              vec3_scale(cam_y, screen_y*half_screen_h)));

            vec3 ray_origin = cam_p;
            vec3 ray_dir = vec3_normalize(vec3_sub(screen_p, cam_p));

            vec3 color = materials[0].color;

            f32 hit_d = f32_max;
            for (u32 plane_id = 0;
                 plane_id < array_count(planes);
                 ++plane_id) {
                plane p = planes[plane_id]; 
                f32 tolerance = 0.0001f;
                f32 denom = vec3_dot(p.n, ray_dir);
                if ((denom < -tolerance) > (denom > tolerance)) {
                    f32 d = (-p.d - vec3_dot(p.n, ray_origin)) / denom;
                    if ((d > 0.0f) && (d < hit_d)) {
                        hit_d = d;
                        color = materials[p.mat_id].color;
                    }
                }
            }

            color = vec3_scale(color, 255.0f);

            u32 out_color = pack_rgba4x8(color.r, color.g, color.b, 0xFF);
            *out++ = out_color;
        }
    }
    write_image(image, "output.bmp");
    return(0);
}
