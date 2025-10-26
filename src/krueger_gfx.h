#ifndef KRUEGER_GFX_H
#define KRUEGER_GFX_H

#define mask_alpha(x) (((x) >> 24) & 0xFF)
#define mask_red(x)   (((x) >> 16) & 0xFF)
#define mask_green(x) (((x) >>  8) & 0xFF)
#define mask_blue(x)  (((x) >>  0) & 0xFF)
#define pack_rgba32(r, g, b, a) ((a << 24) | (r << 16) | (g << 8) | (b << 0))

#endif // KRUEGER_GFX_H
