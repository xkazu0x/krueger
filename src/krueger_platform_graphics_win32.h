#ifndef KRUEGER_PLATFORM_GRAPHICS_WIN32_H
#define KRUEGER_PLATFORM_GRAPHICS_WIN32_H

typedef struct Win32_Window Win32_Window;
struct Win32_Window{
  HWND hwnd;
  HDC hdc;
  BITMAPINFO bitmap_info;
  Win32_Window *next;
};

typedef struct {
  Arena arena;
  HINSTANCE instance;
  ATOM atom;
  Win32_Window *windows;
  Win32_Window *free_window;
} Win32_Graphics_State;

global Win32_Graphics_State *win32_graphics_state;

internal Platform_Handle
win32_handle_from_window(Win32_Window *window) {
  Platform_Handle result = {(uxx)window};
  return(result);
}

internal Win32_Window *
win32_window_from_handle(Platform_Handle handle) {
  Win32_Window *result = (Win32_Window *)handle.ptr[0];
  return(result);
}

internal Win32_Window *
win32_window_alloc(void) {
  Win32_Window *result = win32_graphics_state->free_window;
  if (result) {
    win32_graphics_state->free_window = win32_graphics_state->free_window->next;
  } else {
    result = push_array(&win32_graphics_state->arena, Win32_Window, 1);
  }
  mem_zero_struct(result);
  return(result);
}

internal void
win32_window_release(Win32_Window *window) {
  ReleaseDC(window->hwnd, window->hdc);
  DestroyWindow(window->hwnd);
  window->next = win32_graphics_state->free_window;
  win32_graphics_state->free_window = window;
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

internal void
platform_graphics_init(void) {
  Arena arena = arena_alloc(MB(64));
  win32_graphics_state = push_array(&arena, Win32_Graphics_State, 1);
  win32_graphics_state->arena = arena;

  win32_graphics_state->instance = GetModuleHandleA(0);
  WNDCLASSEXA window_class = {sizeof(window_class)};
  window_class.style = CS_VREDRAW | CS_HREDRAW;
  window_class.lpfnWndProc = win32_window_proc;
  window_class.hInstance = win32_graphics_state->instance;
  window_class.hIcon = LoadIcon(0, IDI_APPLICATION);
  window_class.hCursor = LoadCursor(0, IDC_ARROW);
  window_class.hbrBackground = CreateSolidBrush(RGB(0, 0, 0));
  window_class.lpszClassName = "krueger_graphical_window";
  win32_graphics_state->atom = RegisterClassExA(&window_class);
}

internal Platform_Handle
platform_window_create(String8 title, s32 width, s32 height) {
  char *window_title = (char *)title.str;

  s32 window_w = width;
  s32 window_h = height;

  s32 monitor_w = GetSystemMetrics(SM_CXSCREEN);
  s32 monitor_h = GetSystemMetrics(SM_CYSCREEN);

  s32 window_x = (monitor_w - window_w)/2;
  s32 window_y = (monitor_h - window_h)/2;

  u32 window_style = WS_OVERLAPPEDWINDOW;
  u32 window_style_ex = 0;

  RECT window_rectangle = {0};
  window_rectangle.left = 0;
  window_rectangle.right = window_w;
  window_rectangle.top = 0;
  window_rectangle.bottom = window_h;
  if (AdjustWindowRect(&window_rectangle, window_style, 0)) {
    window_w = window_rectangle.right - window_rectangle.left;
    window_h = window_rectangle.bottom - window_rectangle.top;
  }

  HWND hwnd = CreateWindowExA(window_style_ex, MAKEINTATOM(win32_graphics_state->atom),
                              window_title, window_style,
                              window_x, window_y,
                              window_w, window_h,
                              0, 0, win32_graphics_state->instance, 0);
  ShowWindow(hwnd, SW_SHOW);

  Win32_Window *window = win32_window_alloc();
  window->hwnd = hwnd;
  window->hdc = GetDC(hwnd);

  Platform_Handle result = win32_handle_from_window(window);
  return(result);
}

internal void
platform_window_destroy(Platform_Handle handle) {
  Win32_Window *window = win32_window_from_handle(handle);
  win32_window_release(window);
}

internal void
platform_render_create(Platform_Handle handle, s32 render_w, s32 render_h) {
  Win32_Window *window = win32_window_from_handle(handle);
  window->bitmap_info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
  window->bitmap_info.bmiHeader.biWidth = render_w;
  window->bitmap_info.bmiHeader.biHeight = -render_h;
  window->bitmap_info.bmiHeader.biPlanes = 1;
  window->bitmap_info.bmiHeader.biBitCount = 32;
  window->bitmap_info.bmiHeader.biCompression = BI_RGB;
}

internal void
platform_render_display(Platform_Handle handle, u32 *buffer, s32 buffer_w, s32 buffer_h) {
  Win32_Window *window = win32_window_from_handle(handle);

  RECT client_rectangle;
  GetClientRect(window->hwnd, &client_rectangle);

  s32 window_w = client_rectangle.right - client_rectangle.left;
  s32 window_h = client_rectangle.bottom - client_rectangle.top;
 
  s32 display_w = (s32)floor_f32((buffer_w*((f32)window_h/(f32)buffer_h)));
  s32 display_h = (s32)floor_f32((buffer_h*((f32)window_w/(f32)buffer_w)));

  s32 offset_x = (window_w - display_w)/2;
  s32 offset_y = (window_h - display_h)/2;

  if (window_w >= display_w) {
    display_h = window_h;
    offset_y = 0;
  } else if (window_h >= display_h) {
    display_w = window_w;
    offset_x = 0;
  }

  StretchDIBits(window->hdc,
                offset_x, offset_y,
                display_w, display_h,
                0, 0, buffer_w, buffer_h,
                buffer, &window->bitmap_info,
                DIB_RGB_COLORS, SRCCOPY);
}

int
main(void) {
  platform_core_init();
  platform_graphics_init();

  s32 window_w = WINDOW_WIDTH;
  s32 window_h = WINDOW_HEIGHT;

  s32 render_w = BACK_BUFFER_WIDTH;
  s32 render_h = BACK_BUFFER_HEIGHT;

  Platform_Handle window = platform_window_create(str8_lit("KRUEGER"), window_w, window_h);
  platform_render_create(window, render_w, render_h);

  Image render_image = image_alloc(render_w, render_h);
  image_fill(render_image, 0xFFFF00FF);

  for (b32 quit = false; !quit;) {
    MSG message;
    while (PeekMessageA(&message, 0, 0, 0, PM_REMOVE)) {
      switch (message.message) {
        case WM_QUIT: {
          quit = true;
        } break;
        default: {
          TranslateMessage(&message);
          DispatchMessageA(&message);
        }
      }
    }

    platform_render_display(window, render_image.pixels, render_image.width, render_image.height);
  }

  platform_window_destroy(window);
  platform_core_shutdown();

  return(0);
}

#endif // KRUEGER_PLATFORM_GRAPHICS_WIN32_H
