#ifndef KRUEGER_PLATFORM_GFX_LINUX_H
#define KRUEGER_PLATFORM_GFX_LINUX_H

#include <X11/Xlib.h>
#include <X11/Xutil.h>

typedef struct {
  Display *display;
  Window window;
  Atom wm_delete_window;
  s32 front_buffer_w;
  s32 front_buffer_h;
  uxx front_buffer_size;
  u32 *front_buffer;
  XImage *image;
} Linux_Gfx_State;

///////////////////////////////////
// NOTE: Linux Gfx Global Variables

global Linux_Gfx_State linux_gfx_state;

////////////////////////////
// NOTE: Linux Gfx Functions

internal Keycode linux_translate_keycode(KeySym keycode);

#endif // KRUEGER_PLATFORM_GFX_LINUX_H
