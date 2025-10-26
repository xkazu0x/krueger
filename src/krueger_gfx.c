#ifndef KRUEGER_GFX_C
#define KRUEGER_GFX_C

internal u32
alpha_linear_blend(u32 dst, u32 src) {
  u8 r0 = mask_red(dst); 
  u8 g0 = mask_green(dst);
  u8 b0 = mask_blue(dst);
  u8 a0 = mask_alpha(dst);

  u8 r1 = mask_red(src); 
  u8 g1 = mask_green(src);
  u8 b1 = mask_blue(src);
  u8 a1 = mask_alpha(src);

  u8 r = lerp_t(u8, r0, r1, a1/255.0f);
  u8 g = lerp_t(u8, g0, g1, a1/255.0f);
  u8 b = lerp_t(u8, b0, b1, a1/255.0f);
  u8 a = a0;

  u32 result = pack_rgba32(r, g, b, a);
  return(result);
}

internal void
fill(Image image, u32 color) {
  for (u32 y = 0; y < image.height; ++y) {
    for (u32 x = 0; x < image.width; ++x) {
      image.pixels[y*image.pitch + x] = color;
    }
  }
}

internal void
draw_rect(Image image,
          s32 x0, s32 y0, 
          s32 x1, s32 y1,
          u32 color) {
  if (x0 > x1) swap_t(s32, x0, x1);
  if (y0 > y1) swap_t(s32, y0, y1);
  s32 min_x = clamp_bot(0, x0);
  s32 min_y = clamp_bot(0, y0);
  s32 max_x = clamp_top(x1, (s32)image.width);
  s32 max_y = clamp_top(y1, (s32)image.height);
  for (s32 y = min_y; y < max_y; ++y) {
    for (s32 x = min_x; x < max_x; ++x) {
      image.pixels[y*image.pitch + x] = color;
    }
  }
}

internal void
draw_circle(Image image,
            s32 cx, s32 cy, s32 r,
            u32 color) {
  r = abs_t(s32, r);
  s32 min_x = clamp_bot(0, cx - r);
  s32 min_y = clamp_bot(0, cy - r);
  s32 max_x = clamp_top(cx + r, (s32)image.width);
  s32 max_y = clamp_top(cy + r, (s32)image.height);
  for (s32 y = min_y; y < max_y; ++y) {
    s32 dy = y - cy;
    for (s32 x = min_x; x < max_x; ++x) {
      s32 dx = x - cx;
      if (dx*dx + dy*dy < r*r) {
        image.pixels[y*image.pitch + x] = color;
      }
    }
  }
}

internal void
draw_line(Image image,
          s32 x0, s32 y0, 
          s32 x1, s32 y1,
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
        if ((y >= 0 && y < (s32)image.height) &&
            (x >= 0 && x < (s32)image.width)) {
          image.pixels[y*image.pitch + x] = color; 
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
        if ((y >= 0 && y < (s32)image.height) &&
            (x >= 0 && x < (s32)image.width)) {
          image.pixels[y*image.pitch + x] = color; 
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
draw_triangle(Image image, 
              s32 x0, s32 y0, 
              s32 x1, s32 y1, 
              s32 x2, s32 y2, 
              u32 color) {
  s32 min_x = clamp_bot(0, min(min(x0, x1), x2));
  s32 min_y = clamp_bot(0, min(min(y0, y1), y2));
  s32 max_x = clamp_top(max(max(x0, x1), x2), (s32)image.width);
  s32 max_y = clamp_top(max(max(y0, y1), y2), (s32)image.height);
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
        image.pixels[y*image.pitch + x] = color;
      }
    }
  }
}

internal void
draw_triangle3c(Image image,
                s32 x0, s32 y0, 
                s32 x1, s32 y1, 
                s32 x2, s32 y2,
                u32 c0, u32 c1, u32 c2) {
  s32 min_x = clamp_bot(0, min(min(x0, x1), x2));
  s32 min_y = clamp_bot(0, min(min(y0, y1), y2));
  s32 max_x = clamp_top(max(max(x0, x1), x2), (s32)image.width);
  s32 max_y = clamp_top(max(max(y0, y1), y2), (s32)image.height);
  s32 x01 = x1 - x0;
  s32 y01 = y1 - y0;
  s32 x02 = x2 - x0;
  s32 y02 = y2 - y0;
  s32 x12 = x2 - x1;
  s32 y12 = y2 - y1;
  s32 x20 = x0 - x2;
  s32 y20 = y0 - y2;
  s32 det = x01*y02 - y01*x02;
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
      if ((w0 >= 0.0f) && (w1 >= 0.0f) && (w2 >= 0.0f)) {
        f32 t0 = (f32)w0/det;
        f32 t1 = (f32)w1/det;
        f32 t2 = (f32)w2/det;
        u8 r0 = mask_red(c0);
        u8 g0 = mask_green(c0);
        u8 b0 = mask_blue(c0);
        u8 a0 = mask_alpha(c0);
        u8 r1 = mask_red(c1);
        u8 g1 = mask_green(c1);
        u8 b1 = mask_blue(c1);
        u8 a1 = mask_alpha(c1);
        u8 r2 = mask_red(c2);
        u8 g2 = mask_green(c2);
        u8 b2 = mask_blue(c2);
        u8 a2 = mask_alpha(c2);
        u32 r = (u32)(r0*t0 + r1*t1 + r2*t2);
        u32 g = (u32)(g0*t0 + g1*t1 + g2*t2);
        u32 b = (u32)(b0*t0 + b1*t1 + b2*t2);
        u32 a = (u32)(a0*t0 + a1*t1 + a2*t2);
        image.pixels[y*image.pitch + x] = pack_rgba32(r, g, b, a);
      }
    }
  }
}

internal void
draw_rect_f32(Image image,
              f32 x0, f32 y0, 
              f32 x1, f32 y1,
              u32 color) {
  if (x0 > x1) swap_t(f32, x0, x1);
  if (y0 > y1) swap_t(f32, y0, y1);
  s32 min_x = (s32)clamp_bot(0, floor_f32(x0));
  s32 min_y = (s32)clamp_bot(0, floor_f32(y0));
  s32 max_x = (s32)clamp_top(ceil_f32(x1), image.width);
  s32 max_y = (s32)clamp_top(ceil_f32(y1), image.height);
  for (s32 y = min_y; y < max_y; ++y) {
    for (s32 x = min_x; x < max_x; ++x) {
      image.pixels[y*image.pitch + x] = color;
    }
  }
}

internal void
draw_circle_f32(Image image,
            f32 cx, f32 cy, f32 r,
            u32 color) {
  r = abs_t(f32, r);
  s32 min_x = (s32)clamp_bot(0, floor_f32(cx - r));
  s32 min_y = (s32)clamp_bot(0, floor_f32(cy - r));
  s32 max_x = (s32)clamp_top(ceil_f32(cx + r), image.width);
  s32 max_y = (s32)clamp_top(ceil_f32(cy + r), image.height);
  for (s32 y = min_y; y < max_y; ++y) {
    f32 dy = round_t(f32, y) - cy;
    for (s32 x = min_x; x < max_x; ++x) {
      f32 dx = round_t(f32, x) - cx;
      if (dx*dx + dy*dy < r*r) {
        image.pixels[y*image.pitch + x] = color;
      }
    }
  }
}

internal void
draw_triangle_f32(Image image,
                  f32 x0, f32 y0,
                  f32 x1, f32 y1,
                  f32 x2, f32 y2,
                  u32 color) {
  s32 min_x = (s32)clamp_bot(0, floor_f32(min(min(x0, x1), x2)));
  s32 min_y = (s32)clamp_bot(0, floor_f32(min(min(y0, y1), y2)));
  s32 max_x = (s32)clamp_top(ceil_f32(max(max(x0, x1), x2)), image.width);
  s32 max_y = (s32)clamp_top(ceil_f32(max(max(y0, y1), y2)), image.height);
  f32 x01 = x1 - x0;
  f32 y01 = y1 - y0;
  f32 x12 = x2 - x1;
  f32 y12 = y2 - y1;
  f32 x20 = x0 - x2;
  f32 y20 = y0 - y2;
  f32 bias0 = (((y01 == 0.0f) && (x01 > 0.0f)) || (y01 < 0.0f)) ? 0.0f : -0.0001f;
  f32 bias1 = (((y12 == 0.0f) && (x12 > 0.0f)) || (y12 < 0.0f)) ? 0.0f : -0.0001f;
  f32 bias2 = (((y20 == 0.0f) && (x20 > 0.0f)) || (y20 < 0.0f)) ? 0.0f : -0.0001f;
  for (s32 y = min_y; y < max_y; ++y) {
    f32 dy0 = round_t(f32, y) - y0;
    f32 dy1 = round_t(f32, y) - y1;
    f32 dy2 = round_t(f32, y) - y2;
    for (s32 x = min_x; x < max_x; ++x) {
      f32 dx0 = round_t(f32, x) - x0;
      f32 dx1 = round_t(f32, x) - x1;
      f32 dx2 = round_t(f32, x) - x2;
      f32 w0 = x01*dy0 - y01*dx0 + bias0;
      f32 w1 = x12*dy1 - y12*dx1 + bias1;
      f32 w2 = x20*dy2 - y20*dx2 + bias2;
      if ((w0 >= 0.0f) && (w1 >= 0.0f) && (w2 >= 0.0f)) {
        image.pixels[y*image.pitch + x] = color;
      }
    }
  }
}

internal void
draw_triangle3c_f32(Image image,
                    f32 x0, f32 y0,
                    f32 x1, f32 y1,
                    f32 x2, f32 y2, 
                    u32 c0, u32 c1, u32 c2) {
  s32 min_x = (s32)clamp_bot(0, floor_f32(min(min(x0, x1), x2)));
  s32 min_y = (s32)clamp_bot(0, floor_f32(min(min(y0, y1), y2)));
  s32 max_x = (s32)clamp_top(ceil_f32(max(max(x0, x1), x2)), image.width);
  s32 max_y = (s32)clamp_top(ceil_f32(max(max(y0, y1), y2)), image.height);
  f32 x01 = x1 - x0;
  f32 y01 = y1 - y0;
  f32 x02 = x2 - x0;
  f32 y02 = y2 - y0;
  f32 x12 = x2 - x1;
  f32 y12 = y2 - y1;
  f32 x20 = x0 - x2;
  f32 y20 = y0 - y2;
  f32 det = x01*y02 - y01*x02;
  f32 bias0 = (((y01 == 0.0f) && (x01 > 0.0f)) || (y01 < 0.0f)) ? 0.0f : -0.0001f;
  f32 bias1 = (((y12 == 0.0f) && (x12 > 0.0f)) || (y12 < 0.0f)) ? 0.0f : -0.0001f;
  f32 bias2 = (((y20 == 0.0f) && (x20 > 0.0f)) || (y20 < 0.0f)) ? 0.0f : -0.0001f;
  for (s32 y = min_y; y < max_y; ++y) {
    f32 dy0 = round_t(f32, y) - y0;
    f32 dy1 = round_t(f32, y) - y1;
    f32 dy2 = round_t(f32, y) - y2;
    for (s32 x = min_x; x < max_x; ++x) {
      f32 dx0 = round_t(f32, x) - x0;
      f32 dx1 = round_t(f32, x) - x1;
      f32 dx2 = round_t(f32, x) - x2;
      f32 w0 = x01*dy0 - y01*dx0 + bias0;
      f32 w1 = x12*dy1 - y12*dx1 + bias1;
      f32 w2 = x20*dy2 - y20*dx2 + bias2;
      if ((w0 >= 0.0f) && (w1 >= 0.0f) && (w2 >= 0.0f)) {
        f32 t0 = w0/det;
        f32 t1 = w1/det;
        f32 t2 = w2/det;
        u8 r0 = mask_red(c0);
        u8 g0 = mask_green(c0);
        u8 b0 = mask_blue(c0);
        u8 a0 = mask_alpha(c0);
        u8 r1 = mask_red(c1);
        u8 g1 = mask_green(c1);
        u8 b1 = mask_blue(c1);
        u8 a1 = mask_alpha(c1);
        u8 r2 = mask_red(c2);
        u8 g2 = mask_green(c2);
        u8 b2 = mask_blue(c2);
        u8 a2 = mask_alpha(c2);
        u32 r = (u32)(r0*t0 + r1*t1 + r2*t2);
        u32 g = (u32)(g0*t0 + g1*t1 + g2*t2);
        u32 b = (u32)(b0*t0 + b1*t1 + b2*t2);
        u32 a = (u32)(a0*t0 + a1*t1 + a2*t2);
        image.pixels[y*image.pitch + x] = pack_rgba32(r, g, b, a);
      }
    }
  }
}

internal void
draw_texture(Image dst, Image src, s32 x, s32 y) {
  s32 min_x = clamp_bot(0, x);
  s32 min_y = clamp_bot(0, y);
  s32 max_x = clamp_top(x + src.width, dst.width);
  s32 max_y = clamp_top(y + src.height, dst.height);
  for (s32 dy = min_y; dy < max_y; ++dy) {
    s32 sy = dy - min_y;
    for (s32 dx = min_x; dx < max_x; ++dx) {
      s32 sx = dx - min_x;
      u32 dst_index = dy*dst.pitch + dx;
      u32 src_index = sy*src.pitch + sx;
      u32 dst_color = dst.pixels[dst_index];
      u32 src_color = src.pixels[src_index];
      dst.pixels[dst_index] = alpha_linear_blend(dst_color, src_color);
    }
  }
}

internal void
draw_texturex(Image dst, Image src,
              s32 x, s32 y, f32 sx, f32 sy) {
  f32 sw = sx*src.width;
  f32 sh = sy*src.height;
  s32 min_x = clamp_bot(0, x);
  s32 min_y = clamp_bot(0, y);
  s32 max_x = clamp_top(round_t(u32, x + sw), dst.width);
  s32 max_y = clamp_top(round_t(u32, y + sh), dst.height);
  for (s32 dy = min_y; dy < max_y; ++dy) {
    f32 ny = (dy - min_y)/sh;
    for (s32 dx = min_x; dx < max_x; ++dx) {
      f32 nx = (dx - min_x)/sw;
      u32 dst_index = dy*dst.pitch + dx;
      u32 src_index = (u32)(ny*src.height)*src.pitch + (u32)(nx*src.width);
      u32 dst_color = dst.pixels[dst_index];
      u32 src_color = src.pixels[src_index];
      dst.pixels[dst_index] = alpha_linear_blend(dst_color, src_color);
    }
  }
}

internal void
draw_texturec(Image dst, Image src,
             s32 x, s32 y, u32 c) {
  s32 min_x = clamp_bot(0, x);
  s32 min_y = clamp_bot(0, y);
  s32 max_x = clamp_top(x + src.width, dst.width);
  s32 max_y = clamp_top(y + src.height, dst.height);
  for (s32 dy = min_y; dy < max_y; ++dy) {
    s32 sy = dy - min_y;
    for (s32 dx = min_x; dx < max_x; ++dx) {
      s32 sx = dx - min_x;
      u32 dst_index = dy*dst.pitch + dx;
      u32 src_index = sy*src.pitch + sx;
      u32 dst_color = dst.pixels[dst_index];
      u32 src_color = src.pixels[src_index];
      f32 rt = mask_red(c)/255.0f;
      f32 gt = mask_green(c)/255.0f;
      f32 bt = mask_blue(c)/255.0f;
      f32 at = mask_alpha(c)/255.0f;
      u8 r = (u8)(rt*mask_red(src_color));
      u8 g = (u8)(gt*mask_green(src_color));
      u8 b = (u8)(bt*mask_blue(src_color));
      u8 a = (u8)(at*mask_alpha(src_color));
      src_color = pack_rgba32(r, g, b, a);
      dst.pixels[dst_index] = alpha_linear_blend(dst_color, src_color);
    }
  }
}

internal void
draw_texturexc(Image dst, Image src,
              s32 x, s32 y,
              f32 sx, f32 sy,
              u32 c) {
  f32 sw = sx*src.width;
  f32 sh = sy*src.height;
  s32 min_x = clamp_bot(0, x);
  s32 min_y = clamp_bot(0, y);
  s32 max_x = clamp_top(round_t(u32, x + sw), dst.width);
  s32 max_y = clamp_top(round_t(u32, y + sh), dst.height);
  for (s32 dy = min_y; dy < max_y; ++dy) {
    f32 ny = (dy - min_y)/sh;
    for (s32 dx = min_x; dx < max_x; ++dx) {
      f32 nx = (dx - min_x)/sw;
      u32 dst_index = dy*dst.pitch + dx;
      u32 src_index = (u32)(ny*src.height)*src.pitch + (u32)(nx*src.width);
      u32 dst_color = dst.pixels[dst_index];
      u32 src_color = src.pixels[src_index];
      f32 rt = mask_red(c)/255.0f;
      f32 gt = mask_green(c)/255.0f;
      f32 bt = mask_blue(c)/255.0f;
      f32 at = mask_alpha(c)/255.0f;
      u8 r = (u8)(rt*mask_red(src_color));
      u8 g = (u8)(gt*mask_green(src_color));
      u8 b = (u8)(bt*mask_blue(src_color));
      u8 a = (u8)(at*mask_alpha(src_color));
      src_color = pack_rgba32(r, g, b, a);
      dst.pixels[dst_index] = alpha_linear_blend(dst_color, src_color);
    }
  }
}

internal Image font_get_glyph(Font font, char c);

internal void
draw_text(Image image,
          char *text, s32 x, s32 y, 
          Font font, u32 color) {
  s32 gw = font.glyph_w;
  s32 gh = font.glyph_h;
  s32 gx = x;
  s32 gy = y;
  uxx text_len = cstr_len(text);
  for (uxx i = 0; i < text_len; ++i) {
    char c = text[i];
    switch (c) {
      case '\n': {
        gx = x;
        gy += gh;
      } continue;
      case '\t': {
        gx += 2*gw;
      } continue;
    }
    Image glyph = font_get_glyph(font, c);
    draw_texturec(image, glyph, gx, gy, color);
    gx += gw;
  }
}

internal void
draw_textx(Image image,
           char *text, s32 x, s32 y, 
           Font font, f32 sx, f32 sy, u32 color) {
  s32 gw = round_t(s32, font.glyph_w*sx);
  s32 gh = round_t(s32, font.glyph_h*sy);
  s32 gx = x;
  s32 gy = y;
  uxx text_len = cstr_len(text);
  for (uxx i = 0; i < text_len; ++i) {
    char c = text[i];
    switch (c) {
      case '\n': {
        gx = x;
        gy += gh;
      } continue;
      case '\t': {
        gx += 2*gw;
      } continue;
    }
    Image glyph = font_get_glyph(font, c);
    draw_texturexc(image, glyph, gx, gy, sx, sy, color);
    gx += gw;
  }
}

#endif // KRUEGER_GFX_C
