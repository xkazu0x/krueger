#ifndef KRUEGER_PLATFORM_GFX_LINUX_C
#define KRUEGER_PLATFORM_GFX_LINUX_C

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
platform_create_window(Platform_Window_Desc *desc) {
  char *window_title = desc->window_title;

  s32 window_w = desc->window_w;
  s32 window_h = desc->window_h;

  Display *display = XOpenDisplay(0);
  XAutoRepeatOff(display);

  Window root = XDefaultRootWindow(display);
  Window window = XCreateSimpleWindow(display, root, 0, 0, window_w, window_h, 0, 0, 0);

  u32 event_masks = StructureNotifyMask | FocusChangeMask | KeyPressMask | KeyReleaseMask;
  XSelectInput(display, window, event_masks);

  Atom wm_delete_window = XInternAtom(linux_gfx_state.display, "WM_DELETE_WINDOW", false);
  XSetWMProtocols(display, window, &wm_delete_window, 1);

  XWindowAttributes attributes = {0};
  XGetWindowAttributes(display, window, &attributes);

  XStoreName(display, window, window_title);
  XMapWindow(display, window);

  u32 front_buffer_w = window_w;
  u32 front_buffer_h = window_h;
  uxx front_buffer_size = front_buffer_w*front_buffer_h*sizeof(u32);
  u32 *front_buffer = platform_reserve(front_buffer_size);
  platform_commit(front_buffer, front_buffer_size);

  XImage *image = XCreateImage(display, 
                               attributes.visual, attributes.depth, ZPixmap, 0, 
                               (char *)front_buffer, front_buffer_w, front_buffer_h, 
                               BITS_PER_PIXEL, front_buffer_w*sizeof(u32));
  
  linux_gfx_state.display = display;
  linux_gfx_state.window = window;
  linux_gfx_state.wm_window_delete = wm_window_delete;
  linux_gfx_state.front_buffer_w = front_buffer_w;
  linux_gfx_state.front_buffer_h = front_buffer_h;
  linux_gfx_state.front_buffer = front_buffer;
  linux_gfx_state.image = image;
}

internal void
platform_display_back_buffer(u32 *buffer, s32 buffer_w, s32 buffer_h) {
  s32 front_buffer_w = linux_gfx_state.front_buffer_w;
  s32 front_buffer_h = linux_gfx_state.front_buffer_h;
  u32 *front_buffer = linux_gfx_state.front_buffer;

  for (u32 y = 0; y < front_buffer_h; ++y) {
    for (u32 x = 0; x < front_buffer_w; ++x) {
      u32 nx = x*buffer_w/front_buffer_w;
      u32 ny = y*buffer_h/front_buffer_h;
      front_buffer[y*front_buffer_w + x] = buffer[ny*buffer_w + nx];
    }
  }

  Display *display = linux_gfx_state.display;
  Window window = linux_gfx_state.window;
  XImage *image = linux_gfx_state.image;

  GC context = XCreateGC(display, window, 0, 0);
  XPutImage(display, window, context, image, 0, 0, 0, 0, front_buffer_w, front_buffer_h);
  XFreeGC(display, context);
}

internal void
platform_update_window_events(void) {
  if (buf_len(platform_event_buf) > 0) {
    buf_clear(platform_event_buf);
  }

  while (XPending(display)) {
    XEvent base_event = {0};
    XNextEvent(display, &base_event);
    switch (base_event.type) {
      case ClientMessage: {
        XClientMessageEvent *event = (XClientMessageEvent *)&base_event;
        if ((Atom)event->data.l[0] == wm_delete_window) {
          Platform_Event push_event = {
            .type = PLATFORM_EVENT_QUIT,
          };
          buf_push(platform_event_buf, push_event);
        }
      } break;
      case ConfigureNotify: {
        XConfigureEvent *event = (XConfigureEvent *)&base_event;

        s32 *front_buffer_w = &linux_gfx_state.front_buffer_w;
        s32 *front_buffer_h = &linux_gfx_state.front_buffer_h;
        u32 *front_buffer = linux_gfx_state.front_buffer;
        uxx front_buffer_size = (*front_buffer_w)*(*front_buffer_h)*sizeof(u32);
        platform_release(front_buffer_pixels, front_buffer_size);

        *front_buffer_w = event->width;
        *front_buffer_h = event->height;
        front_buffer_size = (*front_buffer_w)*(*front_buffer_h)*sizeof(u32);
        front_buffer = platform_reserve(front_buffer_size);
        platform_commit(front_buffer, front_buffer_size);

        Display *display = linux_gfx_state.display;
        XImage *image = linux_gfx_state.image;

        image = XCreateImage(display, attributes.visual, attributes.depth, ZPixmap, 0, 
                             (char *)front_buffer, *front_buffer_w, *front_buffer_h, 
                             BITS_PER_PIXEL, (*front_buffer_w)*sizeof(u32));
      } break;
      case KeyPress: 
      case KeyRelease: {
        XKeyEvent *event = (XKeyEvent *)&base_event;
        b32 is_down = (base_event.type == KeyPress);
        KeySym keysym = XLookupKeysym(event, 0);
        Keycode keycode = linux_translate_keycode(keysym);
        Platform_Event push_event = {
          .type = (is_down) ? PLATFORM_EVENT_KEY_PRESS : PLATFORM_EVENT_KEY_RELEASE;
          .keycode = keycode,
        };
        buf_push(platform_event_buf, push_event);
      } break;
      case FocusIn:
      case FocusOut: {
        XFocusChangeEvent *event = (XFocusChangeEvent *)&base_event;
        Display *display = linux_gfx_state.display;
        if (event->type == FocusIn) XAutoRepeatOff(display);
        if (event->type == FocusOut) XAutoRepeatOn(display);
      } break;
    }
  }
}

#endif // KRUEGER_PLATFORM_GFX_LINUX_C
