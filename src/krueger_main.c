#include "krueger_base.h"
#include "krueger_platform.h"
#include "krueger_shared.h"

#include "krueger_base.c"
#include "krueger_platform.c"

#define BITS_PER_PIXEL 32
#include <stdio.h>

internal Image
image_alloc(u32 width, u32 height) {
  Image result = {0};
  result.width = width;
  result.height = height;
  uxx image_size = width*height*sizeof(u32);
  result.pixels = platform_reserve(image_size);
  platform_commit(result.pixels, image_size);
  return(result);
}

internal void
image_release(Image image) {
  uxx image_size = image.width*image.height*sizeof(u32);
  platform_release(image.pixels, image_size);
  image.width = 0;
  image.height = 0;
  image.pixels = 0;
}

internal void
image_copy(Image dst, Image src) {
  for (u32 y = 0; y < dst.height; ++y) {
    for (u32 x = 0; x < dst.width; ++x) {
      u32 nx = x*src.width/dst.width;
      u32 ny = y*src.height/dst.height;
      dst.pixels[y*dst.width + x] = src.pixels[ny*src.width + nx];
    }
  }
}

global Platform_Handle libkrueger;
#define PROC(x) global x##_proc *x;
KRUEGER_PROC_LIST
#undef PROC

internal void
libkrueger_load(char *dst_path, char *src_path) {
  if (platform_copy_file_path(dst_path, src_path)) {
    libkrueger = platform_library_open(dst_path);
    if (!platform_handle_match(libkrueger, PLATFORM_HANDLE_NULL)) {
      #define PROC(x) \
        x = (x##_proc *)platform_library_load_proc(libkrueger, #x); \
        if (!(x)) printf("[ERROR]: reload_libkrueger: failed to load proc from library %s: %s\n", dst_path, #x);
      KRUEGER_PROC_LIST
      #undef PROC
    } else {
      printf("[ERROR]: libkrueger_load: failed to open library: %s\n", dst_path);
    }
  } else {
    printf("[ERROR]: libkrueger_load: failed to copy file path: from [%s] to [%s]\n", src_path, dst_path);
  }
}

internal void
libkrueger_unload(void) {
  if (!platform_handle_match(libkrueger, PLATFORM_HANDLE_NULL)) {
    platform_library_close(libkrueger);
    libkrueger.ptr[0] = 0;
#define PROC(x) x = 0;
    KRUEGER_PROC_LIST
    #undef PROC
  }
}

internal void
process_digital_button(Digital_Button *b, b32 is_down) {
  b32 was_down = b->is_down;
  b->pressed = !was_down && is_down; 
  b->released = was_down && !is_down;
  b->is_down = is_down;
}

internal void
input_reset(Input *input) {
  for (u32 key = 0; key < KEY_MAX; ++key) {
    input->kbd[key].pressed = false;
    input->kbd[key].released = false;
  }
}

#if PLATFORM_WINDOWS
#pragma comment(lib, "user32")
#pragma comment(lib, "gdi32")

global u64 us_res;

internal u64
win32_get_time_us() {
  LARGE_INTEGER large_integer;
  QueryPerformanceCounter(&large_integer);
  u64 result = large_integer.QuadPart*million(1)/us_res;
  return(result);
}

internal LRESULT CALLBACK
win32_window_proc(HWND window, UINT message, WPARAM wparam, LPARAM lparam) {
    LRESULT result = 0;
    switch (message) {
        case WM_CLOSE:
        case WM_DESTROY: {
            PostQuitMessage(0);
        } break;
        default: {
            result = DefWindowProcA(window, message, wparam, lparam);
        }
    }
    return(result);
}

internal Keycode
win32_translate_keycode(u32 keycode) {
  Keycode result = KEY_NULL;
  switch (keycode) {
    case '0': result = KEY_0; break;
    case '1': result = KEY_1; break;
    case '2': result = KEY_2; break;
    case '3': result = KEY_3; break;
    case '4': result = KEY_4; break;
    case '5': result = KEY_5; break;
    case '6': result = KEY_6; break;
    case '7': result = KEY_7; break;
    case '8': result = KEY_8; break;
    case '9': result = KEY_9; break;

    case 'A': result = KEY_A; break;
    case 'B': result = KEY_B; break;
    case 'C': result = KEY_C; break;
    case 'D': result = KEY_D; break;
    case 'E': result = KEY_E; break;
    case 'F': result = KEY_F; break;
    case 'G': result = KEY_G; break;
    case 'H': result = KEY_H; break;
    case 'I': result = KEY_I; break;
    case 'J': result = KEY_J; break;
    case 'K': result = KEY_K; break;
    case 'L': result = KEY_L; break;
    case 'M': result = KEY_M; break;
    case 'N': result = KEY_N; break;
    case 'O': result = KEY_O; break;
    case 'P': result = KEY_P; break;
    case 'Q': result = KEY_Q; break;
    case 'R': result = KEY_R; break;
    case 'S': result = KEY_S; break;
    case 'T': result = KEY_T; break;
    case 'U': result = KEY_U; break;
    case 'V': result = KEY_V; break;
    case 'W': result = KEY_W; break;
    case 'X': result = KEY_X; break;
    case 'Y': result = KEY_Y; break;
    case 'Z': result = KEY_Z; break;

    case VK_UP: result = KEY_UP; break;
    case VK_LEFT: result = KEY_LEFT; break;
    case VK_DOWN: result = KEY_DOWN; break;
    case VK_RIGHT: result = KEY_RIGHT; break;
  }
  return(result);
}

int
main(void) {
  Arena arena = arena_alloc(MB(64)); 

  char *src_lib_path = "..\\build\\libkrueger.dll";
  char *dst_lib_path = "..\\build\\libkruegerx.dll";
  libkrueger_load(dst_lib_path, src_lib_path);

  Krueger_State *krueger_state = 0;
  if (krueger_init) krueger_state = krueger_init();

  char *window_title = "krueger";

  s32 window_width = 960;
  s32 window_height = 720;

  s32 monitor_width = GetSystemMetrics(SM_CXSCREEN);
  s32 monitor_height = GetSystemMetrics(SM_CYSCREEN);

  s32 window_x = (monitor_width - window_width)/2;
  s32 window_y = (monitor_height - window_height)/2;

  s32 fixed_window_width = window_width;
  s32 fixed_window_height = window_height;

  u32 window_style = WS_OVERLAPPEDWINDOW;
  u32 window_style_ex = 0;

  RECT window_rectangle = {0};
  window_rectangle.left = 0;
  window_rectangle.right = window_width;
  window_rectangle.top = 0;
  window_rectangle.bottom = window_height;
  if (AdjustWindowRect(&window_rectangle, window_style, 0)) {
    fixed_window_width = window_rectangle.right - window_rectangle.left;
    fixed_window_height = window_rectangle.bottom - window_rectangle.top;
  }

  HINSTANCE window_instance = GetModuleHandleA(0);

  WNDCLASSA window_class = {0};
  window_class.style = CS_HREDRAW | CS_VREDRAW;
  window_class.lpfnWndProc = win32_window_proc;
  window_class.cbClsExtra = 0;
  window_class.cbWndExtra = 0;
  window_class.hInstance = window_instance;
  window_class.hIcon = LoadIcon(0, IDI_APPLICATION);
  window_class.hCursor = LoadCursor(0, IDC_ARROW);
  window_class.hbrBackground = CreateSolidBrush(RGB(255, 0, 255));
  window_class.lpszMenuName = 0;
  window_class.lpszClassName = "krueger_window_class";
  ATOM window_atom = RegisterClassA(&window_class);
  HWND window = CreateWindowExA(window_style_ex, MAKEINTATOM(window_atom),
                                window_title, window_style,
                                window_x, window_y,
                                fixed_window_width, fixed_window_height,
                                0, 0, window_instance, 0);
  ShowWindow(window, SW_SHOW);

  Image back_buffer = image_alloc(320, 240);

  BITMAPINFO bitmap_info = {0};
  bitmap_info.bmiHeader.biSize = sizeof(bitmap_info.bmiHeader);
  bitmap_info.bmiHeader.biWidth = back_buffer.width;
  bitmap_info.bmiHeader.biHeight = -((s32)back_buffer.height);
  bitmap_info.bmiHeader.biPlanes = 1;
  bitmap_info.bmiHeader.biBitCount = BITS_PER_PIXEL;
  bitmap_info.bmiHeader.biCompression = BI_RGB;

  Input input = {0};
  Clock time = {0};

  LARGE_INTEGER large_integer;
  QueryPerformanceFrequency(&large_integer);
  us_res = large_integer.QuadPart;

  u64 time_start = win32_get_time_us();

  for (b32 quit = false; !quit;) {
    MSG message;
    while (PeekMessageA(&message, 0, 0, 0, PM_REMOVE)) {
      switch (message.message) {
        case WM_QUIT: {
          quit = true;
        } break;
        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
        case WM_KEYDOWN:
        case WM_KEYUP: {
          Keycode keycode = win32_translate_keycode((u32)message.wParam);
          b32 is_down = ((message.lParam & (1 << 31)) == 0);
          process_digital_button(input.kbd + keycode, is_down);
          TranslateMessage(&message);
          DispatchMessageA(&message);
        } break;
        default: {
          TranslateMessage(&message);
          DispatchMessageA(&message);
        }
      }
    }

    if (input.kbd[KEY_Q].pressed) quit = true;
    if (input.kbd[KEY_R].pressed) {
      libkrueger_unload();
      libkrueger_load(dst_lib_path, src_lib_path);
    }

    if (krueger_frame) krueger_frame(krueger_state, back_buffer, input, time);
    input_reset(&input);

    RECT client_rectangle;
    GetClientRect(window, &client_rectangle);
    window_width = client_rectangle.right - client_rectangle.left;
    window_height = client_rectangle.bottom - client_rectangle.top;

    HDC window_device = GetDC(window);
    StretchDIBits(window_device,
                  0, 0, window_width, window_height,
                  0, 0, back_buffer.width, back_buffer.height,
                  back_buffer.pixels,
                  &bitmap_info,
                  DIB_RGB_COLORS, SRCCOPY);
    ReleaseDC(window, window_device);

    u64 time_end = win32_get_time_us();
    time.dt_us = (f32)(time_end - time_start);
    time.dt_ms = time.dt_us/thousand(1);
    time.dt_sec = time.dt_ms/thousand(1);
    time.us += time.dt_us;
    time.ms += time.dt_ms;
    time.sec += time.dt_sec;
    time_start = time_end;
  }
  libkrueger_unload();
  return(0);
}

#elif PLATFORM_LINUX
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <time.h>

internal u64
linux_get_time_us(void) {
  struct timespec clock;
  clock_gettime(CLOCK_MONOTONIC, &clock);
  u64 result = clock.tv_sec*million(1) + clock.tv_nsec/thousand(1); 
  return(result);
}

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

int
main(void) {
  Arena arena = arena_alloc(MB(64));

  char *src_lib_path = "../build/libkrueger.so";
  char *dst_lib_path= "../build/libkruegerx.so";
  libkrueger_load(dst_lib_path, src_lib_path);

  Krueger_State *krueger_state = 0;
  if (krueger_init) krueger_state = krueger_init();

  char *window_title = "krueger";

  s32 window_width = 960;
  s32 window_height = 720;

  Display *display = XOpenDisplay(0);
  XAutoRepeatOff(display);

  Window root = XDefaultRootWindow(display);
  Window window = XCreateSimpleWindow(display, root, 0, 0, window_width, window_height, 0, 0, 0);

  u32 event_masks = StructureNotifyMask | FocusChangeMask | KeyPressMask | KeyReleaseMask;
  XSelectInput(display, window, event_masks);

  Atom wm_delete_window = XInternAtom(display, "WM_DELETE_WINDOW", false);
  XSetWMProtocols(display, window, &wm_delete_window, 1);

  XWindowAttributes attributes = {0};
  XGetWindowAttributes(display, window, &attributes);

  XStoreName(display, window, window_title);
  XMapWindow(display, window);

  Image back_buffer = image_alloc(320, 240);
  Image front_buffer = image_alloc(window_width, window_height); 

  XImage *image = XCreateImage(display, attributes.visual, attributes.depth, ZPixmap, 0, 
                               (char *)front_buffer.pixels, front_buffer.width, front_buffer.height, 
                               BITS_PER_PIXEL, front_buffer.width*sizeof(u32));

  Input input = {0};
  Clock time = {0};

  u64 time_start = linux_get_time_us();

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
          image_release(front_buffer);
          front_buffer = image_alloc(window_width, window_height);
          image = XCreateImage(display, attributes.visual, attributes.depth, ZPixmap, 0, 
                               (char *)front_buffer.pixels, front_buffer.width, front_buffer.height, 
                               BITS_PER_PIXEL, front_buffer.width*sizeof(u32));
        } break;
        case KeyPress: 
        case KeyRelease: {
          XKeyEvent *event = (XKeyEvent *)&base_event;
          KeySym keysym = XLookupKeysym(event, 0);
          Keycode keycode = linux_translate_keycode(keysym);
          b32 is_down = (base_event.type == KeyPress);
          process_digital_button(input.kbd + keycode, is_down);
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
    if (input.kbd[KEY_R].pressed) {
      libkrueger_unload();
      libkrueger_load(dst_lib_path, src_lib_path);
    }

    if (krueger_frame) krueger_frame(krueger_state, back_buffer, input, time);
    input_reset(&input);
    
    image_copy(front_buffer, back_buffer);

    GC context = XCreateGC(display, window, 0, 0);
    XPutImage(display, window, context, image, 0, 0, 0, 0, window_width, window_height);
    XFreeGC(display, context);

    u64 time_end = linux_get_time_us();
    time.dt_us = (f32)(time_end - time_start);
    time.dt_ms = time.dt_us/thousand(1);
    time.dt_sec = time.dt_ms/thousand(1);
    time.us += time.dt_us;
    time.ms += time.dt_ms;
    time.sec += time.dt_sec;
    time_start = time_end;
  }
  XUnmapWindow(display, window);
  XDestroyWindow(display, window);

  XAutoRepeatOn(display);
  XCloseDisplay(display);
  
  libkrueger_unload();
  return(0);
}

#endif // PLATFORM_LINUX
