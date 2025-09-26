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
} BMP_Header;
#pragma pack(pop)

typedef struct {
    u32 width;
    u32 height;
    u32 *pixels;
} Image;

internal u32
get_image_size(Image image) {
    u32 result = image.width*image.height*sizeof(u32);
    return(result);
}

internal Image
alloc_image(u32 width, u32 height) {
    Image result;
    result.width = width;
    result.height = height;
    result.pixels = malloc(get_image_size(result));
    return(result);
}

internal void
write_image(Image image, char *filename) {
    u32 image_size = get_image_size(image);
    BMP_Header header = {
        .type             = 0x4D42,
        .size             = sizeof(BMP_Header) + image_size,
        .reserved1        = 0,
        .reserved2        = 0,
        .offset           = sizeof(BMP_Header),
        .dib_header_size  = sizeof(BMP_Header) - offsetof(BMP_Header, dib_header_size),
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
    f32 scatter; // NOTE: 0 is pure diffuse, 1 is pure specular
    Vector3 emit_color;
    Vector3 refl_color;
} Material;

typedef struct {
    Vector3 n;
    f32 d;
    u32 material_index;
} Plane;

typedef struct {
    Vector3 p;
    f32 r;
    u32 material_index;
} Sphere;

typedef struct {
    u32 material_count;
    Material *materials;
    
    u32 plane_count;
    Plane *planes;

    u32 sphere_count;
    Sphere *spheres;
} World;

internal f32
rand_uni(void) {
    f32 result = (f32)rand() / (f32)RAND_MAX;
    return(result);
}

internal f32
rand_bi(void) {
    f32 result = -1.0f + 2.0f*rand_uni();
    return(result);
}

internal Vector3
ray_cast(World *world, Vector3 ray_origin, Vector3 ray_direction) {
    f32 min_hit_distance = 0.001f;
    f32 tolerance = 0.0001f;

    Vector3 result = {0};
    Vector3 attenuation = make_vector3(1.0f, 1.0f, 1.0f);
    for (u32 ray_count = 0;
         ray_count < 8;
         ++ray_count) {
        f32 hit_distance = f32_max;
        u32 hit_material_index = 0;

        Vector3 next_normal;

        for (u32 plane_index = 0;
        plane_index < world->plane_count;
        ++plane_index) {
            Plane plane = world->planes[plane_index]; 
            f32 denom = vector3_dot(plane.n, ray_direction);
            if ((denom < -tolerance) || (denom > tolerance)) {
                f32 t = (-plane.d - vector3_dot(plane.n, ray_origin)) / denom;
                if ((t > min_hit_distance) && (t < hit_distance)) {
                    hit_distance = t;
                    hit_material_index = plane.material_index;
                    next_normal = plane.n;
                }
            }
        }

        for (u32 sphere_index = 0;
        sphere_index < world->sphere_count;
        ++sphere_index) {
            Sphere sphere = world->spheres[sphere_index]; 

            Vector3 so = vector3_sub(ray_origin, sphere.p);
            f32 a = vector3_dot(ray_direction, ray_direction);
            f32 b = vector3_dot(ray_direction, so)*2.0f;
            f32 c = vector3_dot(so, so) - sphere.r*sphere.r;

            f32 denom = 2.0f*a; 
            f32 root_term = sqrt_f32(b*b - 4.0f*a*c);
            if (root_term > tolerance) {
                f32 tp = (-b + root_term) / denom;
                f32 tn = (-b - root_term) / denom;

                f32 t = tp;
                if ((tn > min_hit_distance) && (tn < tp)) {
                    t = tn;
                }

                if ((t > min_hit_distance) && (t < hit_distance)) {
                    hit_distance = t;
                    hit_material_index = sphere.material_index;
                    next_normal = vector3_normalize(
                        vector3_add(so,
                                    vector3_scale(ray_direction, t)));
                }
            }
        }

        if (hit_material_index) {
            Material material = world->materials[hit_material_index];
            result = vector3_add(result, vector3_hadamard(attenuation, material.emit_color));
            
            f32 cos_attenuation = vector3_dot(vector3_scale(ray_direction, -1.0f), next_normal);
            if (cos_attenuation < 0) cos_attenuation = 0;
            attenuation = vector3_hadamard(attenuation, vector3_scale(material.refl_color, cos_attenuation));

            ray_origin = vector3_add(ray_origin, vector3_scale(ray_direction, hit_distance));
            Vector3 pure_bounce = vector3_sub(ray_direction, 
                                              vector3_scale(next_normal, 
                                                            2.0f*vector3_dot(ray_direction, next_normal)));
            Vector3 random_bounce = vector3_normalize(vector3_add(next_normal, 
                                                                  make_vector3(rand_bi(), 
                                                                               rand_bi(), 
                                                                               rand_bi())));
            ray_direction = vector3_normalize(vector3_lerp(random_bounce, pure_bounce, material.scatter));
        } else {
            Material material = world->materials[hit_material_index];
            result = vector3_add(result, 
                                 vector3_hadamard(attenuation, material.emit_color));
            break;
        }
    }

    return(result);
}

#define array_count(x) (sizeof(x)/sizeof(*(x)))
#define square(x) ((x)*(x))

#define pow_f32(a, b) (f32)pow((a), (b))

internal f32
exact_linear_to_srgb(f32 l) {
    if (l < 0.0f) l = 0.0f;
    if (l > 1.0f) l = 1.0f;
    f32 s = l*12.92f;
    if (l > 0.0031308f) {
        s = 1.055f*pow_f32(l, 1.0f/2.4f) - 0.055f;
    }
    return(s);
}

internal u32
pack_rgba4x8(u8 r, u8 g, u8 b, u8 a) {
    u32 result = ((a << 24) | (r << 16) | (g << 8) | (b << 0));
    return(result);
}

int
main(void) {
    Material materials[] = {
        { .emit_color = make_vector3(0.3f, 0.4f, 0.5f), }, // sky
        { .refl_color = make_vector3(0.5f, 0.5f, 0.5f), }, // floor
        { .refl_color = make_vector3(0.7f, 0.5f, 0.3f), }, 
        { 
            .emit_color = make_vector3(4.0f, 0.0f, 0.0f),
        }, 
        { 
            .refl_color = make_vector3(0.2f, 0.8f, 0.2f), 
            .scatter = 0.7f,
        }, 
        { 
            .refl_color = make_vector3(0.4f, 0.8f, 0.9f), 
            .scatter = 0.85f,
        }, 
    };

    Plane planes[] = {
        {
            .n = make_vector3(0.0f, 0.0f, 1.0f),
            .d = 0.0f,
            .material_index = 1,
        },
    };
    
    Sphere spheres[] = {
        {
            .p = make_vector3(0.0f, 0.0f, 0.0f),
            .r = 1.0f,
            .material_index = 2,
        },
        {
            .p = make_vector3(3.0f, -2.0f, 0.0f),
            .r = 1.0f,
            .material_index = 3,
        },
        {
            .p = make_vector3(-2.0f, -1.0f, 2.0f),
            .r = 1.0f,
            .material_index = 4,
        },
        {
            .p = make_vector3(1.0f, -1.0f, 3.0f),
            .r = 1.0f,
            .material_index = 5,
        },
    };

    World world = {
        .material_count = array_count(materials),
        .materials = materials,
        .plane_count = array_count(planes),
        .planes = planes,
        .sphere_count = array_count(spheres),
        .spheres = spheres,
    };

    Image image = alloc_image(800, 600);

    Vector3 cam_p = make_vector3(0.0f, -10.0f, 1.0f);
    Vector3 cam_z = vector3_normalize(cam_p);
    Vector3 cam_x = vector3_normalize(vector3_cross(make_vector3(0.0f, 0.0f, 1.0f), cam_z));
    Vector3 cam_y = vector3_normalize(vector3_cross(cam_z, cam_x));

    f32 screen_d = 1.0f;
    f32 screen_w = 1.0f;
    f32 screen_h = 1.0f;
    if (image.width > image.height) {
        screen_h = screen_w*((f32)image.height/(f32)image.width);
    } else if (image.height > image.width) {
        screen_w = screen_h*((f32)image.width/(f32)image.height);
    }
    f32 half_screen_w = 0.5f*screen_w;
    f32 half_screen_h = 0.5f*screen_h;
    Vector3 screen_c = vector3_sub(cam_p, vector3_scale(cam_z, screen_d));

    f32 half_pixel_w = 0.5f / image.width;
    f32 half_pixel_h = 0.5f / image.height;
    
    u32 rays_per_pixel = 256;
    u32 *out = image.pixels;
    for (u32 y = 0; y < image.height; ++y) {
        f32 screen_y = -1.0f + 2.0f*((f32)y / (f32)image.height);
        for (u32 x = 0; x < image.width; ++x) {
            f32 screen_x = -1.0f + 2.0f*((f32)x / (f32)image.width);

            Vector3 color = {0};
            f32 contrib = 1.0f / (f32)rays_per_pixel;
            for (u32 ray_index = 0;
                 ray_index < rays_per_pixel;
                 ++ray_index) {
                f32 offset_x = screen_x + rand_bi()*half_pixel_w;
                f32 offset_y = screen_y + rand_bi()*half_pixel_h;
                Vector3 screen_p = vector3_add(screen_c, 
                                               vector3_add(vector3_scale(cam_x, offset_x*half_screen_w), 
                                                           vector3_scale(cam_y, offset_y*half_screen_h)));
                Vector3 ray_origin = cam_p;
                Vector3 ray_direction = vector3_normalize(vector3_sub(screen_p, cam_p));

                Vector3 ray_cast_color = ray_cast(&world, ray_origin, ray_direction);
                color = vector3_add(color, vector3_scale(ray_cast_color, contrib)); 
            }

            color.r = exact_linear_to_srgb(color.r)*255.0f;
            color.g = exact_linear_to_srgb(color.g)*255.0f;
            color.b = exact_linear_to_srgb(color.b)*255.0f;
            u32 out_color = pack_rgba4x8(color.r, color.g, color.b, 0xFF);
            *out++ = out_color;
        }
        printf("\r[INFO]: ray casting: [%.1f%%]", 100.0f*(f32)y/(f32)image.height);
        fflush(stdout);
    }
    char *filename = "krueger.bmp";
    write_image(image, filename);
    printf("\n[INFO]: out: %s\n", filename);
    return(0);
}
