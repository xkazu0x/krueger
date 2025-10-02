#include "base.h"
#include "platform.h"

#include "base.c"
#include "platform.c"

#include <stdio.h>
#include <string.h>

typedef struct {
    u32 width;
    u32 height;
    u32 *pixels;
} Image;

internal Image
alloc_image(u32 width, u32 height) {
    Image result = {
        .width = width,
        .height = height,
        .pixels = malloc(width*height*sizeof(u32)),
    };
    return(result);
}

internal void
image_clear(Image image, u32 color) {
    for (u32 i = 0; i < (image.width*image.height); ++i) {
        image.pixels[i] = color;
    }
}

internal void
draw_rect(u32 *pixels, u32 width, u32 height,
          s32 x0, s32 y0, s32 x1, s32 y1,
          u32 color) {
    if (x0 > x1) swap_t(s32, x0, x1);
    if (y0 > y1) swap_t(s32, y0, y1);
    s32 min_x = clamp_bot(0, x0);
    s32 min_y = clamp_bot(0, y0);
    s32 max_x = clamp_top(x1, (s32)width);
    s32 max_y = clamp_top(y1, (s32)height);
    for (s32 y = min_y; y < max_y; ++y) {
        for (s32 x = min_x; x < max_x; ++x) {
            pixels[y*width + x] = color;
        }
    }
}

internal void
draw_line(u32 *buffer, u32 width, u32 height,
          s32 x0, s32 y0, s32 x1, s32 y1,
          u32 color) {
    if (abs_t(s32, x1 - x0) > abs_t(s32, y1 - y0)) {
        if (x0 > x1) {
            swap_t(s32, x0, x1);
            swap_t(s32, y0, y1);
        }
        s32 dx = x1 - x0;
        s32 dy = y1 - y0;
        s32 dir = (dy < 0) ? -1 : 1;
        dy *= dir;
        if (dx != 0) {
            s32 y = y0;
            s32 d = 2*dy - dx;
            for (s32 x = x0; x <= x1; ++x) {
                if ((y >= 0 && y < (s32)height) &&
                    (x >= 0 && x < (s32)width)) {
                    u32 *dst = buffer + y*width + x;
                    *dst = color; 
                }
                if (d >= 0) {
                    y += dir;
                    d = d - 2*dx;
                }
                d = d + 2*dy;
            }
        }
    } else {
        if (y0 > y1) {
            swap_t(s32, x0, x1);
            swap_t(s32, y0, y1);
        }
        s32 dx = x1 - x0;
        s32 dy = y1 - y0;
        s32 dir = (dx < 0) ? -1 : 1;
        dx *= dir;
        if (dy != 0) {
            s32 x = x0;
            s32 d = 2*dx - dy;
            for (s32 y = y0; y <= y1; ++y) {
                if ((y >= 0 && y < (s32)height) &&
                    (x >= 0 && x < (s32)width)) {
                    u32 *dst = buffer + y*width + x;
                    *dst = color; 
                }
                if (d >= 0) {
                    x += dir;
                    d = d - 2*dy;
                }
                d = d + 2*dx;
            }
        }
    }
}

internal void
fill_triangle(u32 *pixels, u32 width, u32 height,
              s32 x0, s32 y0,
              s32 x1, s32 y1,
              s32 x2, s32 y2,
              u32 color) {
    s32 min_x = clamp_bot(0, min(min(x0, x1), x2));
    s32 min_y = clamp_bot(0, min(min(y0, y1), y2));
    s32 max_x = clamp_top(max(max(x0, x1), x2), (s32)width);
    s32 max_y = clamp_top(max(max(y0, y1), y2), (s32)height);
    s32 x01 = x1 - x0;
    s32 y01 = y1 - y0;
    s32 x12 = x2 - x1;
    s32 y12 = y2 - y1;
    s32 x20 = x0 - x2;
    s32 y20 = y0 - y2;
    for (s32 y = min_y; y < max_y; ++y) {
        s32 dy0 = y - y0;
        s32 dy1 = y - y1;
        s32 dy2 = y - y2;
        for (s32 x = min_x; x < max_x; ++x) {
            s32 dx0 = x - x0;
            s32 dx1 = x - x1;
            s32 dx2 = x - x2;
            s32 w0 = x01*dy0 - y01*dx0;
            s32 w1 = x12*dy1 - y12*dx1;
            s32 w2 = x20*dy2 - y20*dx2;
            if ((w0 >= 0) && (w1 >= 0) && (w2 >= 0)) {
                pixels[y*width + x] = color;
            }
        }
    }
}

internal Vector2
project_to_screen(Vector2 p, f32 w, f32 h) {
    Vector2 result = {
        .x = (p.x + 1.0f)*(f32)w*0.5f,
        .y = (-p.y + 1.0f)*(f32)h*0.5f,
    };
    return(result);
}

typedef struct {
    Vector3 *vert_buf;
    u32 *vert_index_buf;
} Mesh;

internal Mesh
load_obj(const char *filename) {
    Mesh mesh = {0};
    Vector3 *tmp_vert_buf = 0;
    FILE *file = fopen(filename, "r");
    if (file) {
        char line[1<<8];
        while (fscanf(file, "%s", line) != EOF) {
            if (!strcmp(line, "v")) {
                Vector3 v;
                fscanf(file, "%f %f %f\n", &v.x, &v.y, &v.z);
                buf_push(tmp_vert_buf, v);
            } else if (!strcmp(line, "f")) {
                u32 f[3];
                fscanf(file, "%d %d %d\n", &f[0], &f[1], &f[2]);
                buf_push(mesh.vert_index_buf, f[0]);
                buf_push(mesh.vert_index_buf, f[1]);
                buf_push(mesh.vert_index_buf, f[2]);
            }
        }
        fclose(file);
        for (u32 i = 0; i < buf_len(mesh.vert_index_buf); ++i) {
            u32 vert_index = mesh.vert_index_buf[i];
            Vector3 vert = tmp_vert_buf[vert_index-1];
            buf_push(mesh.vert_buf, vert);
        }
        buf_free(tmp_vert_buf);
    } else {
        printf("[ERROR]: Failed to open file: %s\n", filename);
    }
    return(mesh);
}

#define DEFAULT_FONT_WIDTH 8
#define DEFAULT_FONT_HEIGHT 8

global const u8 default_font_glyphs[128][DEFAULT_FONT_HEIGHT][DEFAULT_FONT_WIDTH] = {
    [' '] = {
        { 0, 0, 0, 0, 0, 0, 0, 0 },
        { 0, 0, 0, 0, 0, 0, 0, 0 },
        { 0, 0, 0, 0, 0, 0, 0, 0 },
        { 0, 0, 0, 0, 0, 0, 0, 0 },
        { 0, 0, 0, 0, 0, 0, 0, 0 },
        { 0, 0, 0, 0, 0, 0, 0, 0 },
        { 0, 0, 0, 0, 0, 0, 0, 0 },
        { 0, 0, 0, 0, 0, 0, 0, 0 },
    },
    ['!'] = {
        { 0, 0, 1, 1, 0, 0, 0, 0 },
        { 0, 1, 1, 1, 1, 0, 0, 0 },
        { 0, 1, 1, 1, 1, 0, 0, 0 },
        { 0, 0, 1, 1, 0, 0, 0, 0 },
        { 0, 0, 1, 1, 0, 0, 0, 0 },
        { 0, 0, 0, 0, 0, 0, 0, 0 },
        { 0, 0, 1, 1, 0, 0, 0, 0 },
        { 0, 0, 0, 0, 0, 0, 0, 0 },
    },
    ['A'] = {
        { 0, 1, 1, 1, 1, 0, 0, 0 },
        { 1, 1, 0, 0, 1, 1, 0, 0 },
        { 1, 1, 0, 0, 1, 1, 0, 0 },
        { 1, 1, 1, 1, 1, 1, 0, 0 },
        { 1, 1, 0, 0, 1, 1, 0, 0 },
        { 1, 1, 0, 0, 1, 1, 0, 0 },
        { 1, 1, 0, 0, 1, 1, 0, 0 },
        { 0, 0, 0, 0, 0, 0, 0, 0 },
    },
    ['B'] = {
        { 1, 1, 1, 1, 1, 0, 0, 0 },
        { 1, 1, 0, 0, 1, 1, 0, 0 },
        { 1, 1, 0, 0, 1, 1, 0, 0 },
        { 1, 1, 1, 1, 1, 0, 0, 0 },
        { 1, 1, 0, 0, 1, 1, 0, 0 },
        { 1, 1, 0, 0, 1, 1, 0, 0 },
        { 1, 1, 1, 1, 1, 0, 0, 0 },
        { 0, 0, 0, 0, 0, 0, 0, 0 },
    },
    ['C'] = {
        { 0, 1, 1, 1, 1, 0, 0, 0 },
        { 1, 1, 0, 0, 1, 1, 0, 0 },
        { 1, 1, 0, 0, 0, 0, 0, 0 },
        { 1, 1, 0, 0, 0, 0, 0, 0 },
        { 1, 1, 0, 0, 0, 0, 0, 0 },
        { 1, 1, 0, 0, 1, 1, 0, 0 },
        { 0, 1, 1, 1, 1, 0, 0, 0 },
        { 0, 0, 0, 0, 0, 0, 0, 0 },
    },
    ['D'] = {
        { 1, 1, 1, 1, 0, 0, 0, 0 },
        { 1, 1, 0, 1, 1, 0, 0, 0 },
        { 1, 1, 0, 0, 1, 1, 0, 0 },
        { 1, 1, 0, 0, 1, 1, 0, 0 },
        { 1, 1, 0, 0, 1, 1, 0, 0 },
        { 1, 1, 0, 1, 1, 0, 0, 0 },
        { 1, 1, 1, 1, 0, 0, 0, 0 },
        { 0, 0, 0, 0, 0, 0, 0, 0 },
    },
    ['E'] = {
        { 1, 1, 1, 1, 1, 1, 0, 0 },
        { 1, 1, 0, 0, 0, 0, 0, 0 },
        { 1, 1, 0, 0, 0, 0, 0, 0 },
        { 1, 1, 1, 1, 1, 0, 0, 0 },
        { 1, 1, 0, 0, 0, 0, 0, 0 },
        { 1, 1, 0, 0, 0, 0, 0, 0 },
        { 1, 1, 1, 1, 1, 1, 0, 0 },
        { 0, 0, 0, 0, 0, 0, 0, 0 },
    },
    ['F'] = {
        { 1, 1, 1, 1, 1, 1, 0, 0 },
        { 1, 1, 0, 0, 0, 0, 0, 0 },
        { 1, 1, 0, 0, 0, 0, 0, 0 },
        { 1, 1, 1, 1, 1, 0, 0, 0 },
        { 1, 1, 0, 0, 0, 0, 0, 0 },
        { 1, 1, 0, 0, 0, 0, 0, 0 },
        { 1, 1, 0, 0, 0, 0, 0, 0 },
        { 0, 0, 0, 0, 0, 0, 0, 0 },
    },
    ['G'] = {
        { 0, 1, 1, 1, 1, 0, 0, 0 },
        { 1, 1, 0, 0, 1, 1, 0, 0 },
        { 1, 1, 0, 0, 0, 0, 0, 0 },
        { 1, 1, 0, 1, 1, 1, 0, 0 },
        { 1, 1, 0, 0, 1, 1, 0, 0 },
        { 1, 1, 0, 0, 1, 1, 0, 0 },
        { 0, 1, 1, 1, 1, 0, 0, 0 },
        { 0, 0, 0, 0, 0, 0, 0, 0 },
    },
    ['H'] = {
        { 1, 1, 0, 0, 1, 1, 0, 0 },
        { 1, 1, 0, 0, 1, 1, 0, 0 },
        { 1, 1, 0, 0, 1, 1, 0, 0 },
        { 1, 1, 1, 1, 1, 1, 0, 0 },
        { 1, 1, 0, 0, 1, 1, 0, 0 },
        { 1, 1, 0, 0, 1, 1, 0, 0 },
        { 1, 1, 0, 0, 1, 1, 0, 0 },
        { 0, 0, 0, 0, 0, 0, 0, 0 },
    },
    ['I'] = {
        { 1, 1, 1, 1, 1, 1, 0, 0 },
        { 0, 0, 1, 1, 0, 0, 0, 0 },
        { 0, 0, 1, 1, 0, 0, 0, 0 },
        { 0, 0, 1, 1, 0, 0, 0, 0 },
        { 0, 0, 1, 1, 0, 0, 0, 0 },
        { 0, 0, 1, 1, 0, 0, 0, 0 },
        { 1, 1, 1, 1, 1, 1, 0, 0 },
        { 0, 0, 0, 0, 0, 0, 0, 0 },
    },
    ['J'] = {
        { 0, 1, 1, 1, 1, 1, 0, 0 },
        { 0, 0, 0, 1, 1, 0, 0, 0 },
        { 0, 0, 0, 1, 1, 0, 0, 0 },
        { 0, 0, 0, 1, 1, 0, 0, 0 },
        { 0, 0, 0, 1, 1, 0, 0, 0 },
        { 1, 1, 0, 1, 1, 0, 0, 0 },
        { 0, 1, 1, 1, 0, 0, 0, 0 },
        { 0, 0, 0, 0, 0, 0, 0, 0 },
    },
    ['K'] = {
        { 1, 1, 0, 0, 1, 1, 0, 0 },
        { 1, 1, 0, 1, 1, 0, 0, 0 },
        { 1, 1, 1, 1, 0, 0, 0, 0 },
        { 1, 1, 1, 0, 0, 0, 0, 0 },
        { 1, 1, 1, 1, 0, 0, 0, 0 },
        { 1, 1, 0, 1, 1, 0, 0, 0 },
        { 1, 1, 0, 0, 1, 1, 0, 0 },
        { 0, 0, 0, 0, 0, 0, 0, 0 },
    },
    ['L'] = {
        { 1, 1, 0, 0, 0, 0, 0, 0 },
        { 1, 1, 0, 0, 0, 0, 0, 0 },
        { 1, 1, 0, 0, 0, 0, 0, 0 },
        { 1, 1, 0, 0, 0, 0, 0, 0 },
        { 1, 1, 0, 0, 0, 0, 0, 0 },
        { 1, 1, 0, 0, 0, 0, 0, 0 },
        { 1, 1, 1, 1, 1, 1, 0, 0 },
        { 0, 0, 0, 0, 0, 0, 0, 0 },
    },
    ['M'] = {
        { 1, 1, 0, 0, 0, 1, 1, 0 },
        { 1, 1, 1, 0, 1, 1, 1, 0 },
        { 1, 1, 1, 1, 1, 1, 1, 0 },
        { 1, 1, 0, 1, 0, 1, 1, 0 },
        { 1, 1, 0, 1, 0, 1, 1, 0 },
        { 1, 1, 0, 0, 0, 1, 1, 0 },
        { 1, 1, 0, 0, 0, 1, 1, 0 },
        { 0, 0, 0, 0, 0, 0, 0, 0 },
    },
    ['N'] = {
        { 1, 1, 0, 0, 1, 1, 0, 0 },
        { 1, 1, 0, 0, 1, 1, 0, 0 },
        { 1, 1, 1, 0, 1, 1, 0, 0 },
        { 1, 1, 1, 1, 1, 1, 0, 0 },
        { 1, 1, 0, 1, 1, 1, 0, 0 },
        { 1, 1, 0, 0, 1, 1, 0, 0 },
        { 1, 1, 0, 0, 1, 1, 0, 0 },
        { 0, 0, 0, 0, 0, 0, 0, 0 },
    },
    ['O'] = {
        { 0, 1, 1, 1, 1, 0, 0, 0 },
        { 1, 1, 0, 0, 1, 1, 0, 0 },
        { 1, 1, 0, 0, 1, 1, 0, 0 },
        { 1, 1, 0, 0, 1, 1, 0, 0 },
        { 1, 1, 0, 0, 1, 1, 0, 0 },
        { 1, 1, 0, 0, 1, 1, 0, 0 },
        { 0, 1, 1, 1, 1, 0, 0, 0 },
        { 0, 0, 0, 0, 0, 0, 0, 0 },
    },
    ['P'] = {
        { 1, 1, 1, 1, 1, 0, 0, 0 },
        { 1, 1, 0, 0, 1, 1, 0, 0 },
        { 1, 1, 0, 0, 1, 1, 0, 0 },
        { 1, 1, 1, 1, 1, 0, 0, 0 },
        { 1, 1, 0, 0, 0, 0, 0, 0 },
        { 1, 1, 0, 0, 0, 0, 0, 0 },
        { 1, 1, 0, 0, 0, 0, 0, 0 },
        { 0, 0, 0, 0, 0, 0, 0, 0 },
    },
    ['Q'] = {
        { 0, 1, 1, 1, 1, 0, 0, 0 },
        { 1, 1, 0, 0, 1, 1, 0, 0 },
        { 1, 1, 0, 0, 1, 1, 0, 0 },
        { 1, 1, 0, 0, 1, 1, 0, 0 },
        { 1, 1, 0, 1, 0, 1, 0, 0 },
        { 1, 1, 0, 1, 1, 0, 0, 0 },
        { 0, 1, 1, 0, 1, 1, 0, 0 },
        { 0, 0, 0, 0, 0, 0, 0, 0 },
    },
    ['R'] = {
        { 1, 1, 1, 1, 1, 0, 0, 0 },
        { 1, 1, 0, 0, 1, 1, 0, 0 },
        { 1, 1, 0, 0, 1, 1, 0, 0 },
        { 1, 1, 1, 1, 1, 0, 0, 0 },
        { 1, 1, 0, 1, 1, 0, 0, 0 },
        { 1, 1, 0, 0, 1, 1, 0, 0 },
        { 1, 1, 0, 0, 1, 1, 0, 0 },
        { 0, 0, 0, 0, 0, 0, 0, 0 },
    },
    ['S'] = {
        { 0, 1, 1, 1, 1, 0, 0, 0 },
        { 1, 1, 0, 0, 1, 1, 0, 0 },
        { 1, 1, 0, 0, 0, 0, 0, 0 },
        { 0, 1, 1, 1, 1, 0, 0, 0 },
        { 0, 0, 0, 0, 1, 1, 0, 0 },
        { 1, 1, 0, 0, 1, 1, 0, 0 },
        { 0, 1, 1, 1, 1, 0, 0, 0 },
        { 0, 0, 0, 0, 0, 0, 0, 0 },
    },
    ['T'] = {
        { 1, 1, 1, 1, 1, 1, 0, 0 },
        { 0, 0, 1, 1, 0, 0, 0, 0 },
        { 0, 0, 1, 1, 0, 0, 0, 0 },
        { 0, 0, 1, 1, 0, 0, 0, 0 },
        { 0, 0, 1, 1, 0, 0, 0, 0 },
        { 0, 0, 1, 1, 0, 0, 0, 0 },
        { 0, 0, 1, 1, 0, 0, 0, 0 },
        { 0, 0, 0, 0, 0, 0, 0, 0 },
    },
    ['U'] = {
        { 1, 1, 0, 0, 1, 1, 0, 0 },
        { 1, 1, 0, 0, 1, 1, 0, 0 },
        { 1, 1, 0, 0, 1, 1, 0, 0 },
        { 1, 1, 0, 0, 1, 1, 0, 0 },
        { 1, 1, 0, 0, 1, 1, 0, 0 },
        { 1, 1, 0, 0, 1, 1, 0, 0 },
        { 0, 1, 1, 1, 1, 0, 0, 0 },
        { 0, 0, 0, 0, 0, 0, 0, 0 },
    },
    ['V'] = {
        { 1, 1, 0, 0, 1, 1, 0, 0 },
        { 1, 1, 0, 0, 1, 1, 0, 0 },
        { 1, 1, 0, 0, 1, 1, 0, 0 },
        { 1, 1, 0, 0, 1, 1, 0, 0 },
        { 1, 1, 0, 0, 1, 1, 0, 0 },
        { 0, 1, 1, 1, 1, 0, 0, 0 },
        { 0, 0, 1, 1, 0, 0, 0, 0 },
        { 0, 0, 0, 0, 0, 0, 0, 0 },
    },
    ['W'] = {
        { 1, 1, 0, 0, 0, 1, 1, 0 },
        { 1, 1, 0, 0, 0, 1, 1, 0 },
        { 1, 1, 0, 1, 0, 1, 1, 0 },
        { 1, 1, 0, 1, 0, 1, 1, 0 },
        { 1, 1, 1, 1, 1, 1, 1, 0 },
        { 1, 1, 1, 0, 1, 1, 1, 0 },
        { 1, 1, 0, 0, 0, 1, 1, 0 },
        { 0, 0, 0, 0, 0, 0, 0, 0 },
    },
    ['X'] = {
        { 1, 1, 0, 0, 1, 1, 0, 0 },
        { 1, 1, 0, 0, 1, 1, 0, 0 },
        { 0, 1, 1, 1, 1, 0, 0, 0 },
        { 0, 0, 1, 1, 0, 0, 0, 0 },
        { 0, 1, 1, 1, 1, 0, 0, 0 },
        { 1, 1, 0, 0, 1, 1, 0, 0 },
        { 1, 1, 0, 0, 1, 1, 0, 0 },
        { 0, 0, 0, 0, 0, 0, 0, 0 },
    },
    ['Y'] = {
        { 1, 1, 0, 0, 1, 1, 0, 0 },
        { 1, 1, 0, 0, 1, 1, 0, 0 },
        { 1, 1, 0, 0, 1, 1, 0, 0 },
        { 0, 1, 1, 1, 1, 0, 0, 0 },
        { 0, 0, 1, 1, 0, 0, 0, 0 },
        { 0, 0, 1, 1, 0, 0, 0, 0 },
        { 0, 0, 1, 1, 0, 0, 0, 0 },
        { 0, 0, 0, 0, 0, 0, 0, 0 },
    },
    ['Z'] = {
        { 1, 1, 1, 1, 1, 1, 0, 0 },
        { 0, 0, 0, 0, 1, 1, 0, 0 },
        { 0, 0, 0, 1, 1, 0, 0, 0 },
        { 0, 0, 1, 1, 0, 0, 0, 0 },
        { 0, 1, 1, 0, 0, 0, 0, 0 },
        { 1, 1, 0, 0, 0, 0, 0, 0 },
        { 1, 1, 1, 1, 1, 1, 0, 0 },
        { 0, 0, 0, 0, 0, 0, 0, 0 },
    },
};

internal void
draw_text(u32 *pixels, u32 width, u32 height, 
          const char *text, s32 x, s32 y, 
          const u8 *font_glyphs, u32 glyph_width, u32 glyph_height, 
          u32 font_size, u32 color) {
    uxx text_len = strlen(text);
    for (uxx i = 0; i < text_len; ++i) {
        s32 gx = x + i*glyph_width*font_size;
        s32 gy = y;
        const u8 *glyph = font_glyphs + text[i]*glyph_width*glyph_height*sizeof(u8);
        for (u32 dy = 0; dy < glyph_height; ++dy) {
            for (u32 dx = 0; dx < glyph_width; ++dx) {
                s32 px = gx + dx*font_size;
                s32 py = gy + dy*font_size;
                if (glyph[dy*glyph_width + dx]) {
                    draw_rect(pixels, width, height, px, py, px + font_size, py + font_size, color);
                }
            }
        }
    }
}

int
main(void) {
    char *window_title = "krueger";
    u32 window_width = 800;
    u32 window_height = 600;
    
    Image frame_buffer = alloc_image(window_width, window_height);

    platform_create_window(window_title, window_width, window_height);
    platform_create_window_buffer(frame_buffer.pixels, frame_buffer.width, frame_buffer.height);

    Mesh mesh = load_obj("../res/monkey.obj");
    Vector3 cam_p = make_vector3(0.0f, 0.0f, 0.0f);
            
    char *text = "KRUEGER!        KRUEGER!        KRUEGER!";
    u32 text_size = 2;
    u32 text_color = 0xc1c1c1;
    b32 text_color_switch = false;
    s32 text_len = strlen(text)*text_size*DEFAULT_FONT_WIDTH;
    s32 text_margin = (DEFAULT_FONT_HEIGHT*text_size)/2;

    s32 tick = 0;
    for (b32 quit = false; !quit;) {
        platform_update_window_events();
        for (u32 event_index = 0; 
             event_index < buf_len(event_buf); 
             ++event_index) {
            Event *event = event_buf + event_index;
            switch (event->type) {
                case EVENT_QUIT: {
                    quit = true;
                } break;
                case EVENT_WINDOW_RESIZED: {
                    window_width = event->width;
                    window_height = event->height;
                    platform_destroy_window_buffer();
                    frame_buffer = alloc_image(window_width, window_height);
                    platform_create_window_buffer(frame_buffer.pixels, frame_buffer.width, frame_buffer.height);
                } break;
            }
        }
        
        f32 aspect_ratio = (f32)frame_buffer.height/(f32)frame_buffer.width;
        Matrix4x4 proj = matrix4x4_perspective(90.0f, aspect_ratio, 0.1f, 100.0f);

        Matrix4x4 scale = matrix4x4_scale(make_vector3(1.0f, 1.0f, 1.0f));
        Matrix4x4 rotate = matrix4x4_rotate(make_vector3(1.0f, 1.0f, 0.0f), radians_f32(tick));
        Matrix4x4 translate = matrix4x4_translate(0.0f, 0.0f, 4.0f);
        
        Matrix4x4 model = make_matrix4x4(1.0f);
        model = matrix4x4_mul(scale, model);
        model = matrix4x4_mul(rotate, model);
        model = matrix4x4_mul(translate, model);

        image_clear(frame_buffer, 0);
        
        for (u32 vert_index = 0;
             vert_index < buf_len(mesh.vert_buf);
             vert_index += 3) {
            Vector4 v0 = vector4_from_vector3(mesh.vert_buf[vert_index], 1.0f);
            Vector4 v1 = vector4_from_vector3(mesh.vert_buf[vert_index+1], 1.0f);
            Vector4 v2 = vector4_from_vector3(mesh.vert_buf[vert_index+2], 1.0f);

            v0 = matrix4x4_mul_vector4(model, v0);
            v1 = matrix4x4_mul_vector4(model, v1);
            v2 = matrix4x4_mul_vector4(model, v2);

            Vector3 d01 = vector3_sub(v1.xyz, v0.xyz);
            Vector3 d02 = vector3_sub(v2.xyz, v0.xyz);

            Vector3 normal = vector3_normalize(vector3_cross(d01, d02));
            Vector3 cam_ray = vector3_sub(v0.xyz, cam_p);

            f32 scalar = vector3_dot(normal, cam_ray);

            if (scalar < 0.0f) {
                u32 w = frame_buffer.width;
                u32 h = frame_buffer.height;
                u32 *px = frame_buffer.pixels;

                v0 = matrix4x4_mul_vector4(proj, v0);
                v1 = matrix4x4_mul_vector4(proj, v1);
                v2 = matrix4x4_mul_vector4(proj, v2);

                v0 = vector4_div(v0, v0.w);
                v1 = vector4_div(v1, v1.w);
                v2 = vector4_div(v2, v2.w);

                v0.xy = project_to_screen(v0.xy, w, h);
                v1.xy = project_to_screen(v1.xy, w, h);
                v2.xy = project_to_screen(v2.xy, w, h);

                fill_triangle(px, w, h, v0.x, v0.y, v1.x, v1.y, v2.x, v2.y, 0xc1c1c1);
                draw_line(px, w, h, v0.x, v0.y, v1.x, v1.y, 0x79241f);
                draw_line(px, w, h, v1.x, v1.y, v2.x, v2.y, 0x79241f);
                draw_line(px, w, h, v2.x, v2.y, v0.x, v0.y, 0x79241f);
            }

            if (!(tick % 15)) text_color_switch = !text_color_switch;
            if (text_color_switch) {
                text_color = 0x79241f;
            } else {
                text_color = 0xc1c1c1;
            }
            
            draw_text(frame_buffer.pixels, frame_buffer.width, frame_buffer.height, text,
                      -text_len + ((tick*7) % (frame_buffer.width + text_len)), 
                      frame_buffer.height/2 - DEFAULT_FONT_HEIGHT*text_size - text_margin*2,
                      (u8 *)default_font_glyphs, DEFAULT_FONT_WIDTH, DEFAULT_FONT_HEIGHT,
                      text_size, text_color);
            draw_text(frame_buffer.pixels, frame_buffer.width, frame_buffer.height, text,
                      -text_len + ((tick*8) % (frame_buffer.width + text_len)), 
                      frame_buffer.height/2 - text_margin, 
                      (u8 *)default_font_glyphs, DEFAULT_FONT_WIDTH, DEFAULT_FONT_HEIGHT,
                      text_size, text_color);
            draw_text(frame_buffer.pixels, frame_buffer.width, frame_buffer.height, text,
                      -text_len + ((tick*6) % (frame_buffer.width + text_len)), 
                      frame_buffer.height/2 + DEFAULT_FONT_HEIGHT*text_size, 
                      (u8 *)default_font_glyphs, DEFAULT_FONT_WIDTH, DEFAULT_FONT_HEIGHT,
                      text_size, text_color);
        }

        platform_display_window_buffer(frame_buffer.width, frame_buffer.height);
        tick++;
    }
    platform_destroy_window();
    return(0);
}

// TODO:
// - Fixed Frame Rate
// - Benchmark
// - Rainbow Triangle
// - Texture Mapping
// - Camera View Matrix
// - Depth Buffer
// - Render Text
