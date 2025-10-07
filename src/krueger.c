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
  for (u32 px = 0; px < (image.width*image.height); ++px) {
    image.pixels[px] = color;
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
  Vector3 *vertex_buf;
  u32 *vertex_index_buf;
} Mesh;

internal Mesh
load_obj(const char *filename) {
  Mesh mesh = {0};
  Vector3 *tmp_vertex_buf = 0;
  FILE *file = fopen(filename, "r");
  if (file) {
    char line[1<<8];
    while (fscanf(file, "%s", line) != EOF) {
      if (!strcmp(line, "v")) {
        Vector3 v;
        fscanf(file, "%f %f %f\n", &v.x, &v.y, &v.z);
        buf_push(tmp_vertex_buf, v);
      } else if (!strcmp(line, "f")) {
        u32 f[3];
        fscanf(file, "%d %d %d\n", &f[0], &f[1], &f[2]);
        buf_push(mesh.vertex_index_buf, f[0]);
        buf_push(mesh.vertex_index_buf, f[1]);
        buf_push(mesh.vertex_index_buf, f[2]);
      }
    }
    fclose(file);
    for (u32 i = 0; i < buf_len(mesh.vertex_index_buf); ++i) {
      u32 vertex_index = mesh.vertex_index_buf[i];
      Vector3 v = tmp_vertex_buf[vertex_index-1];
      buf_push(mesh.vertex_buf, v);
    }
    buf_free(tmp_vertex_buf);
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
  ['\"'] = {
    { 1, 1, 0, 1, 1, 0, 0, 0 },
    { 1, 1, 0, 1, 1, 0, 0, 0 },
    { 1, 1, 0, 1, 1, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0 },
  },
  ['#'] = {
    { 0, 1, 1, 0, 1, 1, 0, 0 },
    { 0, 1, 1, 0, 1, 1, 0, 0 },
    { 1, 1, 1, 1, 1, 1, 1, 0 },
    { 0, 1, 1, 0, 1, 1, 0, 0 },
    { 1, 1, 1, 1, 1, 1, 1, 0 },
    { 0, 1, 1, 0, 1, 1, 0, 0 },
    { 0, 1, 1, 0, 1, 1, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0 },
  },
  ['$'] = {
    { 0, 0, 0, 1, 1, 0, 0, 0 },
    { 0, 1, 1, 1, 1, 1, 1, 0 },
    { 1, 1, 0, 1, 0, 0, 0, 0 },
    { 0, 1, 1, 1, 1, 1, 0, 0 },
    { 0, 0, 0, 1, 0, 1, 1, 0 },
    { 1, 1, 1, 1, 1, 1, 0, 0 },
    { 0, 0, 1, 1, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0 },
  },
  ['%'] = {
    { 1, 1, 0, 0, 0, 0, 0, 0 },
    { 1, 1, 0, 0, 1, 1, 0, 0 },
    { 0, 0, 0, 1, 1, 0, 0, 0 },
    { 0, 0, 1, 1, 0, 0, 0, 0 },
    { 0, 1, 1, 0, 0, 0, 0, 0 },
    { 1, 1, 0, 0, 1, 1, 0, 0 },
    { 0, 0, 0, 0, 1, 1, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0 },
  },
  ['&'] = {
    { 0, 1, 1, 1, 0, 0, 0, 0 },
    { 1, 1, 0, 1, 1, 0, 0, 0 },
    { 1, 1, 0, 1, 1, 0, 0, 0 },
    { 0, 1, 1, 1, 0, 0, 0, 0 },
    { 1, 1, 0, 1, 1, 0, 1, 0 },
    { 1, 1, 0, 0, 1, 1, 0, 0 },
    { 0, 1, 1, 1, 0, 1, 1, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0 },
  },
  ['\''] = {
    { 1, 1, 0, 0, 0, 0, 0, 0 },
    { 1, 1, 0, 0, 0, 0, 0, 0 },
    { 1, 1, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0 },
  },
  ['('] = {
    { 0, 0, 0, 1, 1, 0, 0, 0 },
    { 0, 0, 1, 1, 0, 0, 0, 0 },
    { 0, 1, 1, 0, 0, 0, 0, 0 },
    { 0, 1, 1, 0, 0, 0, 0, 0 },
    { 0, 1, 1, 0, 0, 0, 0, 0 },
    { 0, 0, 1, 1, 0, 0, 0, 0 },
    { 0, 0, 0, 1, 1, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0 },
  },
  [')'] = {
    { 0, 1, 1, 0, 0, 0, 0, 0 },
    { 0, 0, 1, 1, 0, 0, 0, 0 },
    { 0, 0, 0, 1, 1, 0, 0, 0 },
    { 0, 0, 0, 1, 1, 0, 0, 0 },
    { 0, 0, 0, 1, 1, 0, 0, 0 },
    { 0, 0, 1, 1, 0, 0, 0, 0 },
    { 0, 1, 1, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0 },
  },
  ['*'] = {
    { 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 1, 1, 0, 0, 0, 0 },
    { 1, 1, 1, 1, 1, 1, 0, 0 },
    { 0, 1, 1, 1, 1, 0, 0, 0 },
    { 1, 1, 1, 1, 1, 1, 0, 0 },
    { 0, 0, 1, 1, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0 },
  },
  ['+'] = {
    { 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 1, 1, 0, 0, 0, 0 },
    { 0, 0, 1, 1, 0, 0, 0, 0 },
    { 1, 1, 1, 1, 1, 1, 0, 0 },
    { 0, 0, 1, 1, 0, 0, 0, 0 },
    { 0, 0, 1, 1, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0 },
  },
  [','] = {
    { 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 1, 1, 0, 0, 0, 0 },
    { 0, 0, 1, 1, 0, 0, 0, 0 },
    { 0, 1, 1, 0, 0, 0, 0, 0 },
  },
  ['-'] = {
    { 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 1, 1, 1, 1, 1, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0 },
  },
  ['.'] = {
    { 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 1, 1, 0, 0, 0, 0 },
    { 0, 0, 1, 1, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0 },
  },
  ['/'] = {
    { 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 1, 1, 0, 0 },
    { 0, 0, 0, 1, 1, 0, 0, 0 },
    { 0, 0, 1, 1, 0, 0, 0, 0 },
    { 0, 1, 1, 0, 0, 0, 0, 0 },
    { 1, 1, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0 },
  },
  ['0'] = {
    { 0, 1, 1, 1, 1, 0, 0, 0 },
    { 1, 1, 0, 0, 1, 1, 0, 0 },
    { 1, 1, 0, 1, 1, 1, 0, 0 },
    { 1, 1, 1, 1, 1, 1, 0, 0 },
    { 1, 1, 1, 0, 1, 1, 0, 0 },
    { 1, 1, 0, 0, 1, 1, 0, 0 },
    { 0, 1, 1, 1, 1, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0 },
  },
  ['1'] = {
    { 0, 0, 1, 1, 0, 0, 0, 0 },
    { 0, 1, 1, 1, 0, 0, 0, 0 },
    { 0, 0, 1, 1, 0, 0, 0, 0 },
    { 0, 0, 1, 1, 0, 0, 0, 0 },
    { 0, 0, 1, 1, 0, 0, 0, 0 },
    { 0, 0, 1, 1, 0, 0, 0, 0 },
    { 1, 1, 1, 1, 1, 1, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0 },
  },
  ['2'] = {
    { 0, 1, 1, 1, 1, 0, 0, 0 },
    { 1, 1, 0, 0, 1, 1, 0, 0 },
    { 0, 0, 0, 0, 1, 1, 0, 0 },
    { 0, 0, 0, 1, 1, 0, 0, 0 },
    { 0, 0, 1, 1, 0, 0, 0, 0 },
    { 0, 1, 1, 0, 0, 0, 0, 0 },
    { 1, 1, 1, 1, 1, 1, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0 },
  },
  ['3'] = {
    { 0, 1, 1, 1, 1, 0, 0, 0 },
    { 1, 1, 0, 0, 1, 1, 0, 0 },
    { 0, 0, 0, 0, 1, 1, 0, 0 },
    { 0, 0, 1, 1, 1, 0, 0, 0 },
    { 0, 0, 0, 0, 1, 1, 0, 0 },
    { 1, 1, 0, 0, 1, 1, 0, 0 },
    { 0, 1, 1, 1, 1, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0 },
  },
  ['4'] = {
    { 0, 0, 0, 0, 1, 1, 0, 0 },
    { 0, 0, 0, 1, 1, 1, 0, 0 },
    { 0, 0, 1, 1, 1, 1, 0, 0 },
    { 0, 1, 1, 0, 1, 1, 0, 0 },
    { 0, 1, 1, 1, 1, 1, 1, 0 },
    { 0, 0, 0, 0, 1, 1, 0, 0 },
    { 0, 0, 0, 0, 1, 1, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0 },
  },
  ['5'] = {
    { 1, 1, 1, 1, 1, 1, 0, 0 },
    { 1, 1, 0, 0, 0, 0, 0, 0 },
    { 1, 1, 1, 1, 1, 0, 0, 0 },
    { 0, 0, 0, 0, 1, 1, 0, 0 },
    { 0, 0, 0, 0, 1, 1, 0, 0 },
    { 1, 1, 0, 0, 1, 1, 0, 0 },
    { 0, 1, 1, 1, 1, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0 },
  },
  ['6'] = {
    { 0, 0, 1, 1, 1, 0, 0, 0 },
    { 0, 1, 1, 0, 0, 0, 0, 0 },
    { 1, 1, 0, 0, 0, 0, 0, 0 },
    { 1, 1, 1, 1, 1, 0, 0, 0 },
    { 1, 1, 0, 0, 1, 1, 0, 0 },
    { 1, 1, 0, 0, 1, 1, 0, 0 },
    { 0, 1, 1, 1, 1, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0 },
  },
  ['7'] = {
    { 1, 1, 1, 1, 1, 1, 0, 0 },
    { 0, 0, 0, 0, 1, 1, 0, 0 },
    { 0, 0, 0, 1, 1, 0, 0, 0 },
    { 0, 0, 1, 1, 0, 0, 0, 0 },
    { 0, 1, 1, 0, 0, 0, 0, 0 },
    { 0, 1, 1, 0, 0, 0, 0, 0 },
    { 0, 1, 1, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0 },
  },
  ['8'] = {
    { 0, 1, 1, 1, 1, 0, 0, 0 },
    { 1, 1, 0, 0, 1, 1, 0, 0 },
    { 1, 1, 0, 0, 1, 1, 0, 0 },
    { 0, 1, 1, 1, 1, 0, 0, 0 },
    { 1, 1, 0, 0, 1, 1, 0, 0 },
    { 1, 1, 0, 0, 1, 1, 0, 0 },
    { 0, 1, 1, 1, 1, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0 },
  },
  ['9'] = {
    { 0, 1, 1, 1, 1, 0, 0, 0 },
    { 1, 1, 0, 0, 1, 1, 0, 0 },
    { 1, 1, 0, 0, 1, 1, 0, 0 },
    { 0, 1, 1, 1, 1, 1, 0, 0 },
    { 0, 0, 0, 0, 1, 1, 0, 0 },
    { 0, 0, 0, 1, 1, 0, 0, 0 },
    { 0, 1, 1, 1, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0 },
  },
  [':'] = {
    { 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 1, 1, 0, 0, 0, 0 },
    { 0, 0, 1, 1, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 1, 1, 0, 0, 0, 0 },
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

internal Matrix4x4
matrix4x4_point_at(Vector3 eye, Vector3 center, Vector3 up) {
  Matrix4x4 result = make_matrix4x4(1.0f);
  center = vector3_normalize(vector3_sub(center, eye));
  up = vector3_normalize(vector3_sub(up, vector3_mul(center, vector3_dot(up, center))));
  Vector3 right = vector3_cross(up, center);
  result.m[0][0] = right.x;
  result.m[0][1] = right.y;
  result.m[0][2] = right.z;
  result.m[1][0] = up.x;
  result.m[1][1] = up.y;
  result.m[1][2] = up.z;
  result.m[2][0] = center.x;
  result.m[2][1] = center.y;
  result.m[2][2] = center.z;
  result.m[3][0] = eye.x;
  result.m[3][1] = eye.y;
  result.m[3][2] = eye.z;
  return(result);
}

internal Matrix4x4
matrix4x4_quick_inverse(Matrix4x4 m) {
  Matrix4x4 result = make_matrix4x4(1.0f);
  result.m[0][0] = m.m[0][0];
  result.m[0][1] = m.m[1][0];
  result.m[0][2] = m.m[2][0];
  result.m[1][0] = m.m[0][1];
  result.m[1][1] = m.m[1][1];
  result.m[1][2] = m.m[2][1];
  result.m[2][0] = m.m[0][2];
  result.m[2][1] = m.m[1][2];
  result.m[2][2] = m.m[2][2];
  result.m[3][0] = -(m.m[3][0]*result.m[0][0] + m.m[3][1]*result.m[1][0] + m.m[3][2]*result.m[2][0]);
  result.m[3][1] = -(m.m[3][0]*result.m[0][1] + m.m[3][1]*result.m[1][1] + m.m[3][2]*result.m[2][1]);
  result.m[3][2] = -(m.m[3][0]*result.m[0][2] + m.m[3][1]*result.m[1][2] + m.m[3][2]*result.m[2][2]);
  return(result);
}

typedef struct {
  b32 is_down;
  b32 pressed;
  b32 released;
} Digital_Button;

global Digital_Button kbd[KEY_MAX];

internal void
process_digital_button(Digital_Button *db, b32 is_down) {
  b32 was_down = db->is_down;
  db->pressed = !was_down && is_down; 
  db->released = was_down && !is_down;
  db->is_down = is_down;
}

internal void
keyboard_reset(void) {
  for (u32 key = 0; key < KEY_MAX; ++key) {
    kbd[key].pressed = false;
    kbd[key].released = false;
  }
}

#if 0
int
main(void) {
  char *window_title = "krueger";
  u32 window_width = 960;
  u32 window_height = 720;

  Image image = alloc_image(window_width, window_height);

  platform_create_window(window_title, window_width, window_height);
  platform_create_window_buffer(image.pixels, image.width, image.height);

  Mesh mesh = load_obj("../res/monkey.obj");

  Vector3 cam_p   = make_vector3(0.0f, 0.0f, 0.0f);
  Vector3 cam_up  = make_vector3(0.0f, 1.0f, 0.0f);
  Vector3 cam_dir = make_vector3(0.0f, 0.0f, 1.0f);

  Vector3 cam_vel = make_vector3(0.0f, 0.0f, 0.0f);
  f32 cam_yaw = 0.0f;

  char *text = "KRUEGER!        KRUEGER!        KRUEGER!";
  u32 text_size = 2;
  u32 text_color = 0xc1c1c1;
  b32 text_color_switch = false;
  s32 text_len = strlen(text)*text_size*DEFAULT_FONT_WIDTH;
  s32 text_margin = (DEFAULT_FONT_HEIGHT*text_size)/2;
  
  char fps_str[256];
  char ms_str[256];
  for (uxx i = 0; i < array_count(fps_str); ++i) {
    fps_str[i] = 0;
    ms_str[i] = 0;
  }

  u64 clock_start = platform_clock();

  s32 tick = 0;
  for (b32 quit = false; !quit;) {
    platform_update_window_events();

    keyboard_reset();
    for (u32 event_index = 0; 
         event_index < buf_len(event_buf); 
         ++event_index) {
      Event *event = event_buf + event_index;
      switch (event->type) {
        case EVENT_QUIT: {
          quit = true;
        } break;
        case EVENT_RESIZE: {
          window_width = event->width;
          window_height = event->height;
          platform_destroy_window_buffer();
          image = alloc_image(window_width, window_height);
          platform_create_window_buffer(image.pixels, image.width, image.height);
        } break;
        case EVENT_KEY_PRESS:
        case EVENT_KEY_RELEASE: {
          Keycode keycode = event->keycode;
          b32 is_down = (event->type == EVENT_KEY_PRESS);
          process_digital_button(kbd + keycode, is_down);
        }
      }
    }

    if (kbd[KEY_Q].pressed) quit = true;

    cam_vel = make_vector3(0.0f, 0.0f, 0.0f);
    if (kbd[KEY_H].is_down) cam_vel.x--;
    if (kbd[KEY_J].is_down) cam_vel = vector3_sub(cam_vel, cam_dir);
    if (kbd[KEY_K].is_down) cam_vel = vector3_add(cam_vel, cam_dir);
    if (kbd[KEY_L].is_down) cam_vel.x++;
    cam_p = vector3_add(cam_p, cam_vel);        

    if (kbd[KEY_A].is_down) cam_yaw--;
    if (kbd[KEY_F].is_down) cam_yaw++;

    Matrix4x4 scale = matrix4x4_scale(make_vector3(1.0f, 1.0f, 1.0f));
    Matrix4x4 rotate = matrix4x4_rotate(make_vector3(1.0f, 1.0f, 0.0f), radians_f32(tick));
    Matrix4x4 translate = matrix4x4_translate(0.0f, 0.0f, 4.0f);

    Matrix4x4 model = make_matrix4x4(1.0f);
    model = matrix4x4_mul(scale, model);
    model = matrix4x4_mul(rotate, model);
    model = matrix4x4_mul(translate, model);

    Vector3 cam_target = make_vector3(0.0f, 0.0f, 1.0f);
    Matrix4x4 cam_rotate = matrix4x4_rotate(make_vector3(0.0f, 1.0f, 0.0f), radians_f32(cam_yaw));
    cam_dir = matrix4x4_mul_vector4(cam_rotate, vector4_from_vector3(cam_target, 1.0f)).xyz;
    cam_target = vector3_add(cam_p, cam_dir);
    Matrix4x4 view = matrix4x4_quick_inverse(matrix4x4_point_at(cam_p, cam_target, cam_up));

    f32 aspect_ratio = (f32)image.height/(f32)image.width;
    Matrix4x4 proj = matrix4x4_perspective(90.0f, aspect_ratio, 0.1f, 100.0f);

    image_clear(image, 0);

    for (u32 vertex_index = 0;
         vertex_index < buf_len(mesh.vertex_buf);
         vertex_index += 3) {
      Vector4 v0 = vector4_from_vector3(mesh.vertex_buf[vertex_index], 1.0f);
      Vector4 v1 = vector4_from_vector3(mesh.vertex_buf[vertex_index+1], 1.0f);
      Vector4 v2 = vector4_from_vector3(mesh.vertex_buf[vertex_index+2], 1.0f);

      v0 = matrix4x4_mul_vector4(model, v0);
      v1 = matrix4x4_mul_vector4(model, v1);
      v2 = matrix4x4_mul_vector4(model, v2);

      Vector3 d01 = vector3_sub(v1.xyz, v0.xyz);
      Vector3 d02 = vector3_sub(v2.xyz, v0.xyz);

      Vector3 normal = vector3_normalize(vector3_cross(d01, d02));
      Vector3 cam_ray = vector3_sub(v0.xyz, cam_p);

      f32 scalar = vector3_dot(normal, cam_ray);

      if (scalar < 0.0f) {
        u32 w = image.width;
        u32 h = image.height;
        u32 *px = image.pixels;

        v0 = matrix4x4_mul_vector4(view, v0);
        v1 = matrix4x4_mul_vector4(view, v1);
        v2 = matrix4x4_mul_vector4(view, v2);

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
    }

    if (!(tick % 15)) text_color_switch = !text_color_switch;
    if (text_color_switch) text_color = 0x79241f; else text_color = 0xc1c1c1;

    draw_text(image.pixels, image.width, image.height, text,
              -text_len + ((tick*7) % (image.width + text_len)), 
              image.height/2 - DEFAULT_FONT_HEIGHT*text_size - text_margin*2,
              (u8 *)default_font_glyphs, DEFAULT_FONT_WIDTH, DEFAULT_FONT_HEIGHT,
              text_size, text_color);
    draw_text(image.pixels, image.width, image.height, text,
              -text_len + ((tick*8) % (image.width + text_len)), 
              image.height/2 - text_margin, 
              (u8 *)default_font_glyphs, DEFAULT_FONT_WIDTH, DEFAULT_FONT_HEIGHT,
              text_size, text_color);
    draw_text(image.pixels, image.width, image.height, text,
              -text_len + ((tick*6) % (image.width + text_len)), 
              image.height/2 + DEFAULT_FONT_HEIGHT*text_size, 
              (u8 *)default_font_glyphs, DEFAULT_FONT_WIDTH, DEFAULT_FONT_HEIGHT,
              text_size, text_color);

    u64 clock_end = platform_clock();
    u64 clock_delta = clock_end - clock_start;
    clock_start = clock_end;

    f32 ms_per_frame = (f32)clock_delta/(f32)MICRO_SEC;
    f32 frames_per_sec = (f32)NANO_SEC/(f32)clock_delta;
    
    if (!(tick % 10)) {
      sprintf(fps_str, "%.2f FPS", frames_per_sec);
      sprintf(ms_str,  "%.2f MS", ms_per_frame);
    }

    s32 offset = 16;
    draw_text(image.pixels, image.width, image.height,
              fps_str, offset, offset,
              (u8 *)default_font_glyphs, DEFAULT_FONT_WIDTH, DEFAULT_FONT_HEIGHT,
              text_size, 0xc1c1c1);
    draw_text(image.pixels, image.width, image.height,
              ms_str, offset, offset + DEFAULT_FONT_HEIGHT*text_size,
              (u8 *)default_font_glyphs, DEFAULT_FONT_WIDTH, DEFAULT_FONT_HEIGHT,
              text_size, 0xc1c1c1);

    platform_display_window_buffer(image.width, image.height);
    tick++;
  }

  platform_destroy_window();
  return(0);
}

internal void
free_image(Image *image) {
  image->width = 0;
  image->height = 0;
  if (image->pixels) {
    free(image->pixels);
  }
}
#else

internal void
draw_triangle(u32 *pixels, u32 width, u32 height,
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
  s32 bias0 = (((y01 == 0) && (x01 > 0)) || (y01 < 0)) ? 0 : -1;
  s32 bias1 = (((y12 == 0) && (x12 > 0)) || (y12 < 0)) ? 0 : -1;
  s32 bias2 = (((y20 == 0) && (x20 > 0)) || (y20 < 0)) ? 0 : -1;
  for (s32 y = min_y; y < max_y; ++y) {
    s32 dy0 = y - y0;
    s32 dy1 = y - y1;
    s32 dy2 = y - y2;
    for (s32 x = min_x; x < max_x; ++x) {
      s32 dx0 = x - x0;
      s32 dx1 = x - x1;
      s32 dx2 = x - x2;
      s32 w0 = x01*dy0 - y01*dx0 + bias0;
      s32 w1 = x12*dy1 - y12*dx1 + bias1;
      s32 w2 = x20*dy2 - y20*dx2 + bias2;
      if ((w0 >= 0) && (w1 >= 0) && (w2 >= 0)) {
        pixels[y*width + x] = color;
      }
    }
  }
}

internal void
draw_triangle_f32(u32 *pixels, u32 width, u32 height,
                  f32 x0, f32 y0,
                  f32 x1, f32 y1,
                  f32 x2, f32 y2,
                  u32 color) {
  u32 min_x = clamp_bot(0, floor_f32(min(min(x0, x1), x2)));
  u32 min_y = clamp_bot(0, floor_f32(min(min(y0, y1), y2)));
  u32 max_x = clamp_top(ceil_f32(max(max(x0, x1), x2)), width);
  u32 max_y = clamp_top(ceil_f32(max(max(y0, y1), y2)), height);
  f32 x01 = x1 - x0;
  f32 y01 = y1 - y0;
  f32 x12 = x2 - x1;
  f32 y12 = y2 - y1;
  f32 x20 = x0 - x2;
  f32 y20 = y0 - y2;
  f32 bias0 = (((y01 == 0.0f) && (x01 > 0.0f)) || (y01 < 0.0f)) ? 0.0f : -0.0001;
  f32 bias1 = (((y12 == 0.0f) && (x12 > 0.0f)) || (y12 < 0.0f)) ? 0.0f : -0.0001;
  f32 bias2 = (((y20 == 0.0f) && (x20 > 0.0f)) || (y20 < 0.0f)) ? 0.0f : -0.0001;
  for (u32 y = min_y; y < max_y; ++y) {
    f32 dy0 = ((f32)y + 0.5f) - y0;
    f32 dy1 = ((f32)y + 0.5f) - y1;
    f32 dy2 = ((f32)y + 0.5f) - y2;
    for (u32 x = min_x; x < max_x; ++x) {
      f32 dx0 = ((f32)x + 0.5f) - x0;
      f32 dx1 = ((f32)x + 0.5f) - x1;
      f32 dx2 = ((f32)x + 0.5f) - x2;
      f32 w0 = x01*dy0 - y01*dx0 + bias0;
      f32 w1 = x12*dy1 - y12*dx1 + bias1;
      f32 w2 = x20*dy2 - y20*dx2 + bias2;
      if ((w0 >= 0.0f) && (w1 >= 0.0f) && (w2 >= 0.0f)) {
        pixels[y*width + x] = color;
      }
    }
  }
}

internal void
test_draw_mesh(Image back_buffer, Mesh mesh, 
               Vector3 cam_p, Vector3 cam_dir, Vector3 cam_up,
               u32 tick, b32 line) {
  Matrix4x4 scale = matrix4x4_scale(make_vector3(1.0f, 1.0f, 1.0f));
  Matrix4x4 rotate = matrix4x4_rotate(make_vector3(1.0f, 1.0f, 0.0f), radians_f32(tick));
  Matrix4x4 translate = matrix4x4_translate(-2.0f, 0.0f, 4.0f);

  Matrix4x4 model = make_matrix4x4(1.0f);
  model = matrix4x4_mul(scale, model);
  model = matrix4x4_mul(rotate, model);
  model = matrix4x4_mul(translate, model);

  Matrix4x4 view = matrix4x4_quick_inverse(matrix4x4_point_at(cam_p, cam_dir, cam_up));

  f32 aspect_ratio = (f32)back_buffer.height/(f32)back_buffer.width;
  Matrix4x4 proj = matrix4x4_perspective(90.0f, aspect_ratio, 0.1f, 100.0f);

  for (u32 vertex_index = 0;
  vertex_index < buf_len(mesh.vertex_buf);
  vertex_index += 3) {
    Vector4 v0 = vector4_from_vector3(mesh.vertex_buf[vertex_index], 1.0f);
    Vector4 v1 = vector4_from_vector3(mesh.vertex_buf[vertex_index+1], 1.0f);
    Vector4 v2 = vector4_from_vector3(mesh.vertex_buf[vertex_index+2], 1.0f);

    v0 = matrix4x4_mul_vector4(model, v0);
    v1 = matrix4x4_mul_vector4(model, v1);
    v2 = matrix4x4_mul_vector4(model, v2);

    Vector3 d01 = vector3_sub(v1.xyz, v0.xyz);
    Vector3 d02 = vector3_sub(v2.xyz, v0.xyz);

    Vector3 normal = vector3_normalize(vector3_cross(d01, d02));
    Vector3 cam_ray = vector3_sub(v0.xyz, cam_p);

    f32 scalar = vector3_dot(normal, cam_ray);

    if (scalar < 0.0f) {
      u32 w = back_buffer.width;
      u32 h = back_buffer.height;
      u32 *px = back_buffer.pixels;

      v0 = matrix4x4_mul_vector4(view, v0);
      v1 = matrix4x4_mul_vector4(view, v1);
      v2 = matrix4x4_mul_vector4(view, v2);

      v0 = matrix4x4_mul_vector4(proj, v0);
      v1 = matrix4x4_mul_vector4(proj, v1);
      v2 = matrix4x4_mul_vector4(proj, v2);

      v0 = vector4_div(v0, v0.w);
      v1 = vector4_div(v1, v1.w);
      v2 = vector4_div(v2, v2.w);

      v0.xy = project_to_screen(v0.xy, w, h);
      v1.xy = project_to_screen(v1.xy, w, h);
      v2.xy = project_to_screen(v2.xy, w, h);

      draw_triangle(px, w, h, v0.x, v0.y, v1.x, v1.y, v2.x, v2.y, 0xc1c1c1);

      if (line) {
        draw_line(px, w, h, v0.x, v0.y, v1.x, v1.y, 0x79241f);
        draw_line(px, w, h, v1.x, v1.y, v2.x, v2.y, 0x79241f);
        draw_line(px, w, h, v2.x, v2.y, v0.x, v0.y, 0x79241f);
      }
    }
  }
}


internal void
test_draw_mesh_f32(Image back_buffer, Mesh mesh, 
                Vector3 cam_p, Vector3 cam_dir, Vector3 cam_up,
                u32 tick, b32 line) {
  Matrix4x4 scale = matrix4x4_scale(make_vector3(1.0f, 1.0f, 1.0f));
  Matrix4x4 rotate = matrix4x4_rotate(make_vector3(1.0f, 1.0f, 0.0f), radians_f32(tick));
  Matrix4x4 translate = matrix4x4_translate(2.0f, 0.0f, 4.0f);

  Matrix4x4 model = make_matrix4x4(1.0f);
  model = matrix4x4_mul(scale, model);
  model = matrix4x4_mul(rotate, model);
  model = matrix4x4_mul(translate, model);

  Matrix4x4 view = matrix4x4_quick_inverse(matrix4x4_point_at(cam_p, cam_dir, cam_up));

  f32 aspect_ratio = (f32)back_buffer.height/(f32)back_buffer.width;
  Matrix4x4 proj = matrix4x4_perspective(90.0f, aspect_ratio, 0.1f, 100.0f);

  for (u32 vertex_index = 0;
  vertex_index < buf_len(mesh.vertex_buf);
  vertex_index += 3) {
    Vector4 v0 = vector4_from_vector3(mesh.vertex_buf[vertex_index], 1.0f);
    Vector4 v1 = vector4_from_vector3(mesh.vertex_buf[vertex_index+1], 1.0f);
    Vector4 v2 = vector4_from_vector3(mesh.vertex_buf[vertex_index+2], 1.0f);

    v0 = matrix4x4_mul_vector4(model, v0);
    v1 = matrix4x4_mul_vector4(model, v1);
    v2 = matrix4x4_mul_vector4(model, v2);

    Vector3 d01 = vector3_sub(v1.xyz, v0.xyz);
    Vector3 d02 = vector3_sub(v2.xyz, v0.xyz);

    Vector3 normal = vector3_normalize(vector3_cross(d01, d02));
    Vector3 cam_ray = vector3_sub(v0.xyz, cam_p);

    f32 scalar = vector3_dot(normal, cam_ray);

    if (scalar < 0.0f) {
      u32 w = back_buffer.width;
      u32 h = back_buffer.height;
      u32 *px = back_buffer.pixels;

      v0 = matrix4x4_mul_vector4(view, v0);
      v1 = matrix4x4_mul_vector4(view, v1);
      v2 = matrix4x4_mul_vector4(view, v2);

      v0 = matrix4x4_mul_vector4(proj, v0);
      v1 = matrix4x4_mul_vector4(proj, v1);
      v2 = matrix4x4_mul_vector4(proj, v2);

      v0 = vector4_div(v0, v0.w);
      v1 = vector4_div(v1, v1.w);
      v2 = vector4_div(v2, v2.w);

      v0.xy = project_to_screen(v0.xy, w, h);
      v1.xy = project_to_screen(v1.xy, w, h);
      v2.xy = project_to_screen(v2.xy, w, h);

      draw_triangle_f32(px, w, h, v0.x, v0.y, v1.x, v1.y, v2.x, v2.y, 0xc1c1c1);
      if (line) {
        draw_line(px, w, h, v0.x, v0.y, v1.x, v1.y, 0x79241f);
        draw_line(px, w, h, v1.x, v1.y, v2.x, v2.y, 0x79241f);
        draw_line(px, w, h, v2.x, v2.y, v0.x, v0.y, 0x79241f);
      }
    }
  }
}

int
main(void) {
  char *window_title = "krueger";

  s32 window_width = 800;
  s32 window_height = 600;

  s32 back_buffer_width = 320;
  s32 back_buffer_height = 240;

  Image front_buffer = alloc_image(window_width, window_height);
  Image back_buffer = alloc_image(back_buffer_width, back_buffer_height);

  Display *display = XOpenDisplay(0);
  Window root = XDefaultRootWindow(display);
  Window window = XCreateSimpleWindow(display, root, 0, 0, window_width, window_height, 0, 0, 0);

  XStoreName(display, window, window_title);
  XMapWindow(display, window);

  u32 event_masks = StructureNotifyMask | FocusChangeMask | KeyPressMask | KeyReleaseMask;
  XSelectInput(display, window, event_masks);

  Atom wm_delete_window = XInternAtom(display, "WM_DELETE_WINDOW", false);
  XSetWMProtocols(display, window, &wm_delete_window, 1);
  
  XWindowAttributes attributes = {0};
  XGetWindowAttributes(display, window, &attributes);

  XImage *image = XCreateImage(display, attributes.visual, attributes.depth, ZPixmap, 0, 
                               (char *)front_buffer.pixels, front_buffer.width, front_buffer.height, 
                               32, front_buffer.width*sizeof(u32));

  Mesh mesh = load_obj("../res/monkey.obj");

  Vector3 cam_p   = make_vector3(0.0f, 0.0f, 0.0f);
  Vector3 cam_up  = make_vector3(0.0f, 1.0f, 0.0f);
  Vector3 cam_dir = make_vector3(0.0f, 0.0f, 1.0f);

  u32 text_size = 1;
  u32 text_color = 0xc1c1c1;
  
  char fps_str[256];
  char ms_str[256];
  for (uxx i = 0; i < array_count(fps_str); ++i) {
    fps_str[i] = 0;
    ms_str[i] = 0;
  }

  u64 clock_start = platform_clock();

  u32 tick = 0;
  for (b32 quit = false; !quit;) {
    while (XPending(display)) {
      XEvent base_event = {0};
      XNextEvent(display, &base_event);
      switch (base_event.type) {
        case ClientMessage: {
          XClientMessageEvent *event = (XClientMessageEvent *)&base_event;
          if ((Atom)event->data.l[0] == wm_delete_window) {
            quit = true;
          }
        } break;
        case ConfigureNotify: {
          XConfigureEvent *event = (XConfigureEvent *)&base_event;
          window_width = event->width;
          window_height = event->height;
          XDestroyImage(image);
          front_buffer = alloc_image(window_width, window_height);
          image = XCreateImage(display, attributes.visual, attributes.depth, ZPixmap, 0, 
                               (char *)front_buffer.pixels, front_buffer.width, front_buffer.height, 
                               32, front_buffer.width*sizeof(u32));
        } break;
      }
    }
    
    image_clear(back_buffer, 0x000000);

    test_draw_mesh(back_buffer, mesh, cam_p, cam_dir, cam_up, tick, false);
    test_draw_mesh_f32(back_buffer, mesh, cam_p, cam_dir, cam_up, tick, false);

    u64 clock_end = platform_clock();
    u64 clock_delta = clock_end - clock_start;
    clock_start = clock_end;

    f32 ms_per_frame = (f32)clock_delta/(f32)MICRO_SEC;
    f32 frames_per_sec = (f32)NANO_SEC/(f32)clock_delta;
    
    if (!(tick % 5)) {
      sprintf(fps_str, "%.2f FPS", frames_per_sec);
      sprintf(ms_str,  "%.2f MS", ms_per_frame);
    }

    s32 offset = 0;
    draw_text(back_buffer.pixels, back_buffer.width, back_buffer.height,
              fps_str, offset, offset,
              (u8 *)default_font_glyphs, DEFAULT_FONT_WIDTH, DEFAULT_FONT_HEIGHT,
              text_size, text_color);
    draw_text(back_buffer.pixels, back_buffer.width, back_buffer.height,
              ms_str, offset, offset + DEFAULT_FONT_HEIGHT*text_size,
              (u8 *)default_font_glyphs, DEFAULT_FONT_WIDTH, DEFAULT_FONT_HEIGHT,
              text_size, text_color);
  
    // NOTE: nearest-neighbor interpolation
    f32 scale_x = (f32)back_buffer.width/(f32)front_buffer.width;
    f32 scale_y = (f32)back_buffer.height/(f32)front_buffer.height;
    for (u32 y = 0; y < front_buffer.height; ++y) {
      for (u32 x = 0; x < front_buffer.width; ++x) {
        u32 nearest_x = (u32)(x*scale_x);
        u32 nearest_y = (u32)(y*scale_y);
        front_buffer.pixels[y*front_buffer.width + x] = back_buffer.pixels[nearest_y*back_buffer.width + nearest_x];
      }
    }

    GC context = XCreateGC(display, window, 0, 0);
    XPutImage(display, window, context, image, 0, 0, 0, 0, window_width, window_height);
    XFreeGC(display, context);
    ++tick;
  }

  XUnmapWindow(display, window);
  XDestroyWindow(display, window);
  XCloseDisplay(display);

  return(0);
}
#endif

// TODO:
// - Clipping
// - Depth Buffer
// - Texture Mapping
// - Fixed Frame Rate
// - Rainbow Triangle
