#ifndef PLATFORM_LINUX_C
#define PLATFORM_LINUX_C

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include <time.h>

typedef struct {
  Display *display;
  Window window;
  Atom wm_delete_window;
  XImage *image;
} Platform_Linux_State;

global Platform_Linux_State linux_state;

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
platform_create_window(const char *title, u32 width, u32 height) {
  linux_state.display = XOpenDisplay(0);
  linux_state.window = XCreateSimpleWindow(linux_state.display, XDefaultRootWindow(linux_state.display), 0, 0, width, height, 0, 0, 0);

  linux_state.wm_delete_window = XInternAtom(linux_state.display, "WM_DELETE_WINDOW", False);
  XSetWMProtocols(linux_state.display, linux_state.window, &linux_state.wm_delete_window, 1);

  u32 event_masks = StructureNotifyMask | FocusChangeMask | KeyPressMask | KeyReleaseMask;
  XSelectInput(linux_state.display, linux_state.window, event_masks);

  XStoreName(linux_state.display, linux_state.window, title);
  XMapWindow(linux_state.display, linux_state.window);
  XFlush(linux_state.display);
}

internal void
platform_destroy_window(void) {
  XAutoRepeatOn(linux_state.display);
  XUnmapWindow(linux_state.display, linux_state.window);
  XDestroyWindow(linux_state.display, linux_state.window);
  XCloseDisplay(linux_state.display);
}

internal void
platform_create_window_buffer(u32 *buffer, u32 width, u32 height) {
  XWindowAttributes attributes = {0};
  XGetWindowAttributes(linux_state.display, linux_state.window, &attributes);
  linux_state.image = XCreateImage(linux_state.display, attributes.visual, attributes.depth, ZPixmap, 0, 
                                   (char *)buffer, width, height, 32, width*sizeof(u32));
}

internal void
platform_display_window_buffer(u32 width, u32 height) {
  GC gc = XCreateGC(linux_state.display, linux_state.window, 0, 0);
  XPutImage(linux_state.display, linux_state.window, gc, linux_state.image, 0, 0, 0, 0, width, height);
  XFreeGC(linux_state.display, gc);
}

internal void
platform_destroy_window_buffer(void) {
  XDestroyImage(linux_state.image);
}

internal void
platform_update_window_events(void) {
  buf_clear(event_buf);
  while (XPending(linux_state.display)) {
    XEvent base_event = {0};
    XNextEvent(linux_state.display, &base_event);
    switch (base_event.type) {
      case ClientMessage: {
        XClientMessageEvent *event = (XClientMessageEvent *)&base_event;
        if ((Atom)event->data.l[0] == linux_state.wm_delete_window) {
          Event push_event = {0};
          push_event.type = EVENT_QUIT;
          buf_push(event_buf, push_event);
        }
      } break;
      case ConfigureNotify: {
        XConfigureEvent *event = (XConfigureEvent *)&base_event;
        Event push_event = {0};
        push_event.type = EVENT_RESIZE;
        push_event.width = event->width;
        push_event.height = event->height;
        buf_push(event_buf, push_event);
      } break;
      case KeyPress: 
      case KeyRelease: {
        XKeyEvent *event = (XKeyEvent *)&base_event;
        Event push_event = {0};
        if (base_event.type == KeyPress) {
          push_event.type = EVENT_KEY_PRESS;
        } else {
          push_event.type = EVENT_KEY_RELEASE;
        }
        KeySym keysym = XLookupKeysym(event, 0);
        push_event.keycode = linux_translate_keycode(keysym);
        buf_push(event_buf, push_event);
      }
      case FocusIn:
      case FocusOut: {
        XFocusChangeEvent *event = (XFocusChangeEvent *)&base_event;
        if (event->type == FocusIn) XAutoRepeatOff(linux_state.display);
        if (event->type == FocusOut) XAutoRepeatOn(linux_state.display);
      } break;
    }
  }
}

internal s64
platform_clock(void) {
  struct timespec clock;
  clock_gettime(CLOCK_MONOTONIC, &clock);
  s64 result = (s64)clock.tv_sec*NANO_SEC + (s64)clock.tv_nsec; 
  return(result);
}

#endif // PLATFORM_LINUX_C
