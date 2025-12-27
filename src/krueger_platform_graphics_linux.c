#ifndef KRUEGER_PLATFORM_GRAPHICS_LINUX_C
#define KRUEGER_PLATFORM_GRAPHICS_LINUX_C

////////////////////////
// NOTE: Linux Functions

internal Platform_Handle
_linux_handle_from_window(_Linux_Window *window) {
  Platform_Handle result = {(uxx)window};
  return(result);
}

internal _Linux_Window *
_linux_window_from_handle(Platform_Handle handle) {
  _Linux_Window *result = (_Linux_Window *)handle.ptr[0];
  return(result);
}

internal _Linux_Window *
_linux_window_from_xwnd(Window xwnd) {
  _Linux_Window *result = 0;
  for (each_node(_Linux_Window, window, _lnx_gfx_state->first_window)) {
    if(window->xwnd == xwnd) {
      result = window;
      break;
    }
  }
  return(result);
}

internal _Linux_Window *
_linux_window_alloc(void) {
  _Linux_Window *result = _lnx_gfx_state->first_free_window;
  if (result) {
    stack_pop(_lnx_gfx_state->first_free_window);
  } else {
    result = push_struct(_lnx_gfx_state->arena, _Linux_Window);
  }
  mem_zero_struct(result);
  dll_push_back(_lnx_gfx_state->first_window,
                _lnx_gfx_state->last_window,
                result);
  return(result);
}

internal void
_linux_window_release(_Linux_Window *window) {
  XDestroyWindow(_lnx_gfx_state->display, window->xwnd);
  dll_remove(_lnx_gfx_state->first_window,
             _lnx_gfx_state->last_window,
             window);
  stack_push(_lnx_gfx_state->first_free_window, window);
}

internal Keycode
_linux_keycode_from_keysym(KeySym keysym) {
  Keycode result = KEY_NULL;
  switch (keysym) {
    case XK_Escape: {result = KEY_ESCAPE;} break;
    case XK_BackSpace: {result = KEY_SPACE;} break;

    case XK_F1: {result = KEY_F1;} break;
    case XK_F2: {result = KEY_F2;} break;
    case XK_F3: {result = KEY_F3;} break;
    case XK_F4: {result = KEY_F4;} break;
    case XK_F5: {result = KEY_F5;} break;
    case XK_F6: {result = KEY_F6;} break;
    case XK_F7: {result = KEY_F7;} break;
    case XK_F8: {result = KEY_F8;} break;
    case XK_F9: {result = KEY_F9;} break;
    case XK_F10: {result = KEY_F10;} break;
    case XK_F11: {result = KEY_F11;} break;
    case XK_F12: {result = KEY_F12;} break;
    case XK_F13: {result = KEY_F13;} break;
    case XK_F14: {result = KEY_F14;} break;
    case XK_F15: {result = KEY_F15;} break;
    case XK_F16: {result = KEY_F16;} break;
    case XK_F17: {result = KEY_F17;} break;
    case XK_F18: {result = KEY_F18;} break;
    case XK_F19: {result = KEY_F19;} break;
    case XK_F20: {result = KEY_F20;} break;
    case XK_F21: {result = KEY_F21;} break;
    case XK_F22: {result = KEY_F22;} break;
    case XK_F23: {result = KEY_F23;} break;
    case XK_F24: {result = KEY_F24;} break;

    case XK_Up: {result = KEY_UP;} break;
    case XK_Left: {result = KEY_LEFT;} break;
    case XK_Down: {result = KEY_DOWN;} break;
    case XK_Right: {result = KEY_RIGHT;} break;

    case XK_0: {result = KEY_0;} break;
    case XK_1: {result = KEY_1;} break;
    case XK_2: {result = KEY_2;} break;
    case XK_3: {result = KEY_3;} break;
    case XK_4: {result = KEY_4;} break;
    case XK_5: {result = KEY_5;} break;
    case XK_6: {result = KEY_6;} break;
    case XK_7: {result = KEY_7;} break;
    case XK_8: {result = KEY_8;} break;
    case XK_9: {result = KEY_9;} break;

    case XK_A: case XK_a: {result = KEY_A;} break;
    case XK_B: case XK_b: {result = KEY_B;} break;
    case XK_C: case XK_c: {result = KEY_C;} break;
    case XK_D: case XK_d: {result = KEY_D;} break;
    case XK_E: case XK_e: {result = KEY_E;} break;
    case XK_F: case XK_f: {result = KEY_F;} break;
    case XK_G: case XK_g: {result = KEY_G;} break;
    case XK_H: case XK_h: {result = KEY_H;} break;
    case XK_I: case XK_i: {result = KEY_I;} break;
    case XK_J: case XK_j: {result = KEY_J;} break;
    case XK_K: case XK_k: {result = KEY_K;} break;
    case XK_L: case XK_l: {result = KEY_L;} break;
    case XK_M: case XK_m: {result = KEY_M;} break;
    case XK_N: case XK_n: {result = KEY_N;} break;
    case XK_O: case XK_o: {result = KEY_O;} break;
    case XK_P: case XK_p: {result = KEY_P;} break;
    case XK_Q: case XK_q: {result = KEY_Q;} break;
    case XK_R: case XK_r: {result = KEY_R;} break;
    case XK_S: case XK_s: {result = KEY_S;} break;
    case XK_T: case XK_t: {result = KEY_T;} break;
    case XK_U: case XK_u: {result = KEY_U;} break;
    case XK_V: case XK_v: {result = KEY_V;} break;
    case XK_W: case XK_w: {result = KEY_W;} break;
    case XK_X: case XK_x: {result = KEY_X;} break;
    case XK_Y: case XK_y: {result = KEY_Y;} break;
    case XK_Z: case XK_z: {result = KEY_Z;} break;
  }
  return(result);
}

///////////////////////////
// NOTE: Platform Functions

internal void
platform_graphics_init(void) {
  Arena *arena = arena_alloc();
  _lnx_gfx_state = push_struct(arena, _Linux_Graphics_State);
  _lnx_gfx_state->arena = arena;
  _lnx_gfx_state->display = XOpenDisplay(0);
  _lnx_gfx_state->wm_delete_window = XInternAtom(_lnx_gfx_state->display, "WM_DELETE_WINDOW", 0);

  _lnx_gfx_state->gfx_info.refresh_rate = 60.0f;
}

internal Platform_Graphics_Info
platform_get_graphics_info(void) {
  return(_lnx_gfx_state->gfx_info);
}

internal Platform_Handle
platform_window_open(String8 name, s32 width, s32 height) {
  s32 window_w = width;
  s32 window_h = height;

  s32 screen = DefaultScreen(_lnx_gfx_state->display);
  s32 monitor_w = DisplayWidth(_lnx_gfx_state->display, screen);
  s32 monitor_h = DisplayHeight(_lnx_gfx_state->display, screen);

  s32 window_x = (monitor_w - window_w)/2;
  s32 window_y = (monitor_h - window_h)/2;

  s32 window_attribs_mask = CWBackPixel | CWEventMask | CWColormap;

  XSetWindowAttributes window_attribs = {
    .background_pixel = 0xff000000,
    .event_mask = StructureNotifyMask | KeyPressMask | KeyReleaseMask | PointerMotionMask,
  };

  Window xwnd = XCreateWindow(_lnx_gfx_state->display,
                             XDefaultRootWindow(_lnx_gfx_state->display), 
                             window_x, window_y, window_w, window_h,
                             0, CopyFromParent, InputOutput, CopyFromParent,
                             window_attribs_mask, &window_attribs);

  XSetWMProtocols(_lnx_gfx_state->display, xwnd, &_lnx_gfx_state->wm_delete_window, 1);

  Temp scratch = scratch_begin(0, 0);
  String8 window_name = str8_copy(scratch.arena, name);
  XStoreName(_lnx_gfx_state->display, xwnd, (char *)window_name.str);
  scratch_end(scratch);

  _Linux_Window *window = _linux_window_alloc();
  window->xwnd = xwnd;

  Platform_Handle result = _linux_handle_from_window(window);
  return(result);
}

internal void
platform_window_close(Platform_Handle handle) {
  if (platform_handle_is_valid(handle)) {
    _Linux_Window *window = _linux_window_from_handle(handle);
    _linux_window_release(window);
  }
}

internal void
platform_window_show(Platform_Handle handle) {
  if (platform_handle_is_valid(handle)) {
    _Linux_Window *window = _linux_window_from_handle(handle);
    XMapWindow(_lnx_gfx_state->display, window->xwnd);
  }
}

internal void
platform_window_blit(Platform_Handle handle, u32 *buffer, s32 buffer_w, s32 buffer_h) {
}

internal b32
platform_window_is_fullscreen(Platform_Handle handle) {
  b32 result = true;
  return(result);
}

internal void
platform_window_set_fullscreen(Platform_Handle handle, b32 fullscreen) {
}

internal Rect2
platform_get_window_client_rect(Platform_Handle handle) {
  Rect2 result = {0};
  if (platform_handle_is_valid(handle)) {
    _Linux_Window *window = _linux_window_from_handle(handle);
    XWindowAttributes attribs;
    XGetWindowAttributes(_lnx_gfx_state->display, window->xwnd, &attribs);
    result.min.x = (f32)attribs.x;
    result.min.y = (f32)attribs.y;
    result.max.x = (f32)(result.min.x + attribs.width);
    result.max.y = (f32)(result.min.y + attribs.height);
  }
  return(result);
}

internal Platform_Event_List
platform_get_event_list(Arena *arena) {
  Platform_Event_List event_list = {0};
  while(XPending(_lnx_gfx_state->display)) {
    XEvent xevent;
    XNextEvent(_lnx_gfx_state->display, &xevent);
    switch(xevent.type) {
      case ClientMessage: {
        if((Atom)xevent.xclient.data.l[0] == _lnx_gfx_state->wm_delete_window) {
          _Linux_Window *window = _linux_window_from_xwnd(xevent.xclient.window);
          Platform_Event *event = platform_event_list_push(arena, &event_list, PLATFORM_EVENT_WINDOW_CLOSE);
          event->window = _linux_handle_from_window(window);
        }
      } break;
      case KeyPress:
      case KeyRelease: {
        _Linux_Window *window = _linux_window_from_xwnd(xevent.xclient.window);
        KeySym keysym = XLookupKeysym(&xevent.xkey, 0);
        Platform_Event_Type event_type = (xevent.type == KeyPress) ? 
          PLATFORM_EVENT_KEY_PRESS : PLATFORM_EVENT_KEY_RELEASE;
        Platform_Event *event = platform_event_list_push(arena, &event_list, event_type);
        event->window = _linux_handle_from_window(window);
        event->keycode = _linux_keycode_from_keysym(keysym);
      } break;
    }
  }
  return(event_list);
}

#endif // KRUEGER_PLATFORM_GRAPHICS_LINUX_C
