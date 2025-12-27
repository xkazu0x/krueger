#ifndef KRUEGER_PLATFORM_GRAPHICS_LINUX_H
#define KRUEGER_PLATFORM_GRAPHICS_LINUX_H

/////////////////////////////
// NOTE: Includes / Libraries

#include <X11/Xlib.h>

//////////////
// NOTE: Types

typedef struct _Linux_Window _Linux_Window;
struct _Linux_Window {
  _Linux_Window *next;
  _Linux_Window *prev;
  Window xwnd;
};

typedef struct {
  Arena *arena;
  Display *display;
  Atom wm_delete_window;
  Platform_Graphics_Info gfx_info;
  _Linux_Window *first_window;
  _Linux_Window *last_window;
  _Linux_Window *first_free_window;
} _Linux_Graphics_State;

//////////////////////
// NOTE: Linux Globals

global _Linux_Graphics_State *_lnx_gfx_state;

////////////////////////
// NOTE: Linux Functions

internal Platform_Handle  _linux_handle_from_window(_Linux_Window *window);
internal _Linux_Window    *_linux_window_from_handle(Platform_Handle handle);
internal _Linux_Window    *_linux_window_from_xwnd(Window xwnd);
internal _Linux_Window    *_linux_window_alloc(void);
internal void             _linux_window_release(_Linux_Window *window);
internal Keycode          _linux_keycode_from_keysym(KeySym keysym);

#endif // KRUEGER_PLATFORM_GRAPHICS_LINUX_H
