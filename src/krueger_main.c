#include "krueger_base.h"
#include "krueger_base.c"
#include "krueger_shared.h"

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <dlfcn.h>
#include <time.h>
#include <stdio.h>

internal Keycode
linux_translate_keycode(KeySym keycode) {
  Keycode result = KEY_NULL;
  switch (keycode) {
    case XK_0: result = KEY_0; break;
    case XK_1: result = KEY_1; break;
    case XK_2: result = KEY_2; break;
    case XK_3: result = KEY_3; break;
    case XK_4: result = KEY_4; break;
    case XK_5: result = KEY_5; break;
    case XK_6: result = KEY_6; break;
    case XK_7: result = KEY_7; break;
    case XK_8: result = KEY_8; break;
    case XK_9: result = KEY_9; break;

    case XK_A: case XK_a: result = KEY_A; break;
    case XK_B: case XK_b: result = KEY_B; break;
    case XK_C: case XK_c: result = KEY_C; break;
    case XK_D: case XK_d: result = KEY_D; break;
    case XK_E: case XK_e: result = KEY_E; break;
    case XK_F: case XK_f: result = KEY_F; break;
    case XK_G: case XK_g: result = KEY_G; break;
    case XK_H: case XK_h: result = KEY_H; break;
    case XK_I: case XK_i: result = KEY_I; break;
    case XK_J: case XK_j: result = KEY_J; break;
    case XK_K: case XK_k: result = KEY_K; break;
    case XK_L: case XK_l: result = KEY_L; break;
    case XK_M: case XK_m: result = KEY_M; break;
    case XK_N: case XK_n: result = KEY_N; break;
    case XK_O: case XK_o: result = KEY_O; break;
    case XK_P: case XK_p: result = KEY_P; break;
    case XK_Q: case XK_q: result = KEY_Q; break;
    case XK_R: case XK_r: result = KEY_R; break;
    case XK_S: case XK_s: result = KEY_S; break;
    case XK_T: case XK_t: result = KEY_T; break;
    case XK_U: case XK_u: result = KEY_U; break;
    case XK_V: case XK_v: result = KEY_V; break;
    case XK_W: case XK_w: result = KEY_W; break;
    case XK_X: case XK_x: result = KEY_X; break;
    case XK_Y: case XK_y: result = KEY_Y; break;
    case XK_Z: case XK_z: result = KEY_Z; break;

    case XK_Up: result = KEY_UP; break;
    case XK_Left: result = KEY_LEFT; break;
    case XK_Down: result = KEY_DOWN; break;
    case XK_Right: result = KEY_RIGHT; break;
  }
  return(result);
}

internal void
linux_process_digital_button(Digital_Button *b, b32 is_down) {
  b32 was_down = b->is_down;
  b->pressed = !was_down && is_down; 
  b->released = was_down && !is_down;
  b->is_down = is_down;
}

internal void
linux_reset_input(Input *input) {
  for (u32 key = 0; key < KEY_MAX; ++key) {
    input->kbd[key].pressed = false;
    input->kbd[key].released = false;
  }
}

internal u64
linux_get_wall_clock(void) {
  struct timespec clock;
  clock_gettime(CLOCK_MONOTONIC, &clock);
  u64 result = clock.tv_sec*billion(1) + clock.tv_nsec; 
  return(result);
}

global Input input;
global void *libkrueger;
#define PROC(x) global x##_proc *x;
SHARED_PROC_LIST;
#undef PROC

internal void
linux_reload_libkrueger(char *lib_str) {
  if (libkrueger) dlclose(libkrueger);
  libkrueger = dlopen(lib_str, RTLD_NOW);
  if (libkrueger) {
  #define PROC(x) \
    x = dlsym(libkrueger, #x); \
    if (!x) printf("[ERROR]: %s\n", dlerror());
  SHARED_PROC_LIST;
  #undef PROC
  } else {
    printf("[ERROR]: %s\n", dlerror());
  }
}

int
main(void) {
  char *libkrueger_str = "../build/libkrueger.so";
  linux_reload_libkrueger(libkrueger_str);

  char *window_title = "krueger";

  s32 window_width = 800;
  s32 window_height = 600;

  s32 back_buffer_width = 320;
  s32 back_buffer_height = 240;

  Image front_buffer = alloc_image(window_width, window_height);
  Image back_buffer = alloc_image(back_buffer_width, back_buffer_height);

  Display *display = XOpenDisplay(0);
  XAutoRepeatOff(display);

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

  u64 clock_start = linux_get_wall_clock();
  u64 clock_delta = 0;

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
        case KeyPress: 
        case KeyRelease: {
          XKeyEvent *event = (XKeyEvent *)&base_event;
          KeySym keysym = XLookupKeysym(event, 0);
          Keycode keycode = linux_translate_keycode(keysym);
          b32 is_down = (base_event.type == KeyPress);
          linux_process_digital_button(input.kbd + keycode, is_down);
        } break;
        case FocusIn:
        case FocusOut: {
          XFocusChangeEvent *event = (XFocusChangeEvent *)&base_event;
          if (event->type == FocusIn) XAutoRepeatOff(display);
          if (event->type == FocusOut) XAutoRepeatOn(display);
        } break;
      }
    }

    if (input.kbd[KEY_Q].pressed) quit = true;
    if (input.kbd[KEY_R].pressed) linux_reload_libkrueger(libkrueger_str);

    if (update_and_render) update_and_render(back_buffer, input, clock_delta);
    linux_reset_input(&input);

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
    
    // NOTE: display front buffer
    GC context = XCreateGC(display, window, 0, 0);
    XPutImage(display, window, context, image, 0, 0, 0, 0, window_width, window_height);
    XFreeGC(display, context);
    
    // NOTE: compute time
    u64 clock_end = linux_get_wall_clock();
    clock_delta = clock_end - clock_start;
    clock_start = clock_end;
  }

  XUnmapWindow(display, window);
  XDestroyWindow(display, window);

  XAutoRepeatOn(display);
  XCloseDisplay(display);

  dlclose(libkrueger);
  return(0);
}

// TODO:
// - Fixed Frame Rate
// - Load Bitmap
// - Font Struct
// - Clipping
// - Texture Mapping
// - Depth Buffer
