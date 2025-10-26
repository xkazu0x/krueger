#ifndef KRUEGER_GFX_H
#define KRUEGER_GFX_H

#define mask_alpha(x) (((x) >> 24) & 0xFF)
#define mask_red(x)   (((x) >> 16) & 0xFF)
#define mask_green(x) (((x) >>  8) & 0xFF)
#define mask_blue(x)  (((x) >>  0) & 0xFF)
#define pack_rgba32(r, g, b, a) ((a << 24) | (r << 16) | (g << 8) | (b << 0))

internal u32 alpha_linear_blend(u32 dst, u32 src);

internal void draw_rect(Image image, s32 x0, s32 y0, s32 x1, s32 y1, u32 color);
internal void draw_circle(Image image, s32 cx, s32 cy, s32 r, u32 color);
internal void draw_line(Image image, s32 x0, s32 y0, s32 x1, s32 y1, u32 color);
internal void draw_triangle(Image image, s32 x0, s32 y0, s32 x1, s32 y1, s32 x2, s32 y2, u32 color);
internal void draw_triangle3c(Image image, s32 x0, s32 y0, s32 x1, s32 y1, s32 x2, s32 y2, u32 c0, u32 c1, u32 c2);

internal void draw_rect_f32(Image image, f32 x0, f32 y0, f32 x1, f32 y1, u32 color);
internal void draw_circle_f32(Image image, f32 cx, f32 cy, f32 r, u32 color);
internal void draw_triangle_f32(Image image, f32 x0, f32 y0, f32 x1, f32 y1, f32 x2, f32 y2, u32 color);
internal void draw_triangle3c_f32(Image image, f32 x0, f32 y0, f32 x1, f32 y1, f32 x2, f32 y2, u32 c0, u32 c1, u32 c2);

internal void draw_texture(Image dst, Image src, s32 x, s32 y);
internal void draw_texturec(Image dst, Image src, s32 x, s32 y, u32 c);
internal void draw_char(Image image, char c, s32 x, s32 y, Font font, u32 color);
internal void draw_text(Image image, char *text, s32 x, s32 y, Font font, u32 color);

#endif // KRUEGER_GFX_H
