#ifndef KRUEGER_PLATFORM_GRAPHICS_WIN32_C
#define KRUEGER_PLATFORM_GRAPHICS_WIN32_C

//////////////////////////
// NOTE: Windows Functions

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
win32_window_from_hwnd(HWND hwnd) {
  Win32_Window *result = 0;
  for (Win32_Window *window = _win32_graphics_state->first_window;
       window != 0;
       window = window->next) {
    if (window->hwnd == hwnd) {
      result = window;
      break;
    }
  }
  return(result);
}

internal Win32_Window *
win32_window_alloc(void) {
  Win32_Window *result = _win32_graphics_state->free_window;
  if (result) {
    stack_pop(_win32_graphics_state->free_window);
  } else {
    result = push_array(_win32_graphics_state->arena, Win32_Window, 1);
  }
  mem_zero_struct(result);
  dll_push_back(_win32_graphics_state->first_window,
                _win32_graphics_state->last_window,
                result);
  result->last_placement.length = sizeof(WINDOWPLACEMENT);
  return(result);
}

internal void
win32_window_release(Win32_Window *window) {
  ReleaseDC(window->hwnd, window->hdc);
  DestroyWindow(window->hwnd);
  dll_remove(_win32_graphics_state->first_window,
             _win32_graphics_state->last_window,
             window);
  stack_push(_win32_graphics_state->free_window, window);
}

internal Platform_Event *
win32_push_event(Platform_Event_Type type, Win32_Window *window) {
  Platform_Event *event = push_array(_win32_event_arena, Platform_Event, 1);
  queue_push(_win32_event_list.first, _win32_event_list.last, event);
  event->type = type;
  event->window = win32_handle_from_window(window);
  return(event);
}

internal LRESULT CALLBACK
win32_window_proc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) {
  LRESULT result = 0;
  if (_win32_event_arena) {
    Win32_Window *window = win32_window_from_hwnd(hwnd);
    switch (message) {
      case WM_CLOSE: {
        win32_push_event(PLATFORM_EVENT_WINDOW_CLOSE, window);
      } break;
      case WM_SYSKEYDOWN:
      case WM_SYSKEYUP: {
        if (wparam == VK_F4) {
          result = DefWindowProcW(hwnd, message, wparam, lparam);
        }
      } // NOTE: fallthrough
      case WM_KEYDOWN:
      case WM_KEYUP: {
        // b32 was_down = lparam&bit31);
        b32 is_down = !(lparam&bit32);
        Platform_Event_Type type = (is_down) ?
          PLATFORM_EVENT_KEY_PRESS :
          PLATFORM_EVENT_KEY_RELEASE;
        Platform_Event *event = win32_push_event(type, window);
        event->keycode = _win32_graphics_state->key_table[wparam&bitmask8];
      } break;
      default: {
        result = DefWindowProcW(hwnd, message, wparam, lparam);
      } break;
    }
  } else {
    result = DefWindowProcW(hwnd, message, wparam, lparam);
  }
  return(result);
}

///////////////////////////
// NOTE: Platform Functions

internal void
platform_graphics_init(void) {
  Arena *arena = arena_alloc();
  _win32_graphics_state = push_array(arena, Win32_Graphics_State, 1);
  _win32_graphics_state->arena = arena;

  _win32_graphics_state->instance = GetModuleHandleW(0);

  WNDCLASSEXW window_class = {.cbSize = sizeof(window_class)};
  window_class.style = CS_VREDRAW | CS_HREDRAW;
  window_class.lpfnWndProc = win32_window_proc;
  window_class.hInstance = _win32_graphics_state->instance;
  window_class.hIcon = LoadIconW(0, IDI_APPLICATION);
  window_class.hCursor = LoadCursorW(0, IDC_ARROW);
  window_class.hbrBackground = CreateSolidBrush(RGB(0, 0, 0));
  window_class.lpszClassName = L"krueger_graphical_window";

  _win32_graphics_state->atom = RegisterClassExW(&window_class);

  DEVMODEW devmode = {.dmSize = sizeof(devmode)};
  if (EnumDisplaySettingsW(0, ENUM_CURRENT_SETTINGS, &devmode)) {
    _win32_graphics_state->graphics_info.refresh_rate = (f32)devmode.dmDisplayFrequency;
  }

  _win32_graphics_state->key_table[VK_ESCAPE] = KEY_ESCAPE;
  _win32_graphics_state->key_table[VK_SPACE] = KEY_SPACE;

  _win32_graphics_state->key_table[VK_UP] = KEY_UP;
  _win32_graphics_state->key_table[VK_LEFT] = KEY_LEFT;
  _win32_graphics_state->key_table[VK_DOWN] = KEY_DOWN;
  _win32_graphics_state->key_table[VK_RIGHT] = KEY_RIGHT;

  for (u32 i = VK_F1, j = KEY_F1; i <= VK_F24; ++i, ++j) {
    _win32_graphics_state->key_table[i] = j;
  }

  for (u32 i = '0', j = KEY_0; i <= '9'; ++i, ++j) {
    _win32_graphics_state->key_table[i] = j;
  }

  for (u32 i = 'A', j = KEY_A; i <= 'Z'; ++i, ++j) {
    _win32_graphics_state->key_table[i] = j;
  }
}

internal Platform_Graphics_Info
platform_get_graphics_info(void) {
  return(_win32_graphics_state->graphics_info);
}

internal Platform_Handle
platform_window_open(String8 name, s32 width, s32 height) {
  Temp scratch = scratch_begin(0, 0);
  String16 window_name = str16_from_str8(scratch.arena, name);

  s32 window_w = width;
  s32 window_h = height;

  u32 window_style = WS_OVERLAPPEDWINDOW;
  u32 window_style_ex = WS_EX_APPWINDOW;

  RECT window_rectangle = {0};
  window_rectangle.left = 0;
  window_rectangle.right = window_w;
  window_rectangle.top = 0;
  window_rectangle.bottom = window_h;

  if (AdjustWindowRect(&window_rectangle, window_style, 0)) {
    window_w = window_rectangle.right - window_rectangle.left;
    window_h = window_rectangle.bottom - window_rectangle.top;
  }

  s32 monitor_w = GetSystemMetrics(SM_CXSCREEN);
  s32 monitor_h = GetSystemMetrics(SM_CYSCREEN);

  s32 window_x = (monitor_w - window_w)/2;
  s32 window_y = (monitor_h - window_h)/2;

  HWND hwnd = CreateWindowExW(window_style_ex, MAKEINTATOM(_win32_graphics_state->atom),
                              window_name.str, window_style,
                              window_x, window_y, window_w, window_h,
                              0, 0, _win32_graphics_state->instance, 0);

  Win32_Window *window = win32_window_alloc();
  window->hwnd = hwnd;
  window->hdc = GetDC(hwnd);

  Platform_Handle result = win32_handle_from_window(window);
  scratch_end(scratch);
  return(result);
}

internal void
platform_window_close(Platform_Handle handle) {
  Win32_Window *window = win32_window_from_handle(handle);
  if (window != 0) {
    win32_window_release(window);
  }
}

internal void
platform_window_show(Platform_Handle handle) {
  Win32_Window *window = win32_window_from_handle(handle);
  if (window != 0) {
    ShowWindow(window->hwnd, SW_SHOW);
  }
}

internal void
platform_window_blit(Platform_Handle handle, u32 *buffer, s32 buffer_w, s32 buffer_h) {
  Win32_Window *window = win32_window_from_handle(handle);
  if (window != 0) {
    BITMAPINFO bitmap_info = {0};
    bitmap_info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bitmap_info.bmiHeader.biWidth = buffer_w;
    bitmap_info.bmiHeader.biHeight = -buffer_h;
    bitmap_info.bmiHeader.biPlanes = 1;
    bitmap_info.bmiHeader.biBitCount = 32;
    bitmap_info.bmiHeader.biCompression = BI_RGB;
    
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
                  buffer, &bitmap_info,
                  DIB_RGB_COLORS, SRCCOPY);
  }
}

internal b32
platform_window_is_fullscreen(Platform_Handle handle) {
  b32 result = false;
  Win32_Window *window = win32_window_from_handle(handle);
  if (window != 0) {
    DWORD window_style = GetWindowLongW(window->hwnd, GWL_STYLE);
    result = !(window_style & WS_OVERLAPPEDWINDOW);
  }
  return(result);
}

internal void
platform_window_set_fullscreen(Platform_Handle handle, b32 fullscreen) {
  Win32_Window *window = win32_window_from_handle(handle);
  if (window != 0) {
    DWORD window_style = GetWindowLongW(window->hwnd, GWL_STYLE);
    b32 is_fullscreen_already = platform_window_is_fullscreen(handle);
    if (fullscreen && !is_fullscreen_already) {
      MONITORINFO monitor_info = {.cbSize = sizeof(monitor_info)};
      if (GetWindowPlacement(window->hwnd, &window->last_placement) &&
        GetMonitorInfoW(MonitorFromWindow(window->hwnd, MONITOR_DEFAULTTOPRIMARY), &monitor_info)) {
        SetWindowLongW(window->hwnd, GWL_STYLE, window_style & ~WS_OVERLAPPEDWINDOW);
        SetWindowPos(window->hwnd, HWND_TOP,
                     monitor_info.rcMonitor.left,
                     monitor_info.rcMonitor.top,
                     monitor_info.rcMonitor.right - monitor_info.rcMonitor.left,
                     monitor_info.rcMonitor.bottom - monitor_info.rcMonitor.top,
                     SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
      }
    } else if (!fullscreen && is_fullscreen_already) {
      SetWindowLongPtrW(window->hwnd, GWL_STYLE, window_style | WS_OVERLAPPEDWINDOW);
      SetWindowPlacement(window->hwnd, &window->last_placement);
      SetWindowPos(window->hwnd, 0, 0, 0, 0, 0,
                   SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |
                   SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
    }
  }
}

internal Rect2
platform_get_window_client_rect(Platform_Handle handle) {
  Rect2 result = {0};
  if (platform_handle_is_valid(handle)) {
    Win32_Window *window = win32_window_from_handle(handle);
    RECT client_rect;
    GetClientRect(window->hwnd, &client_rect);
    result.min.x = (f32)client_rect.left;
    result.min.y = (f32)client_rect.top;
    result.max.x = (f32)client_rect.right;
    result.max.y = (f32)client_rect.bottom;
  }
  return(result);
}

internal Platform_Event_List
platform_get_event_list(Arena *arena) {
  _win32_event_arena = arena;
  mem_zero_struct(&_win32_event_list);
  MSG message;
  while (PeekMessageW(&message, 0, 0, 0, PM_REMOVE)) {
    TranslateMessage(&message);
    DispatchMessageW(&message);
  }
  return(_win32_event_list);
}

#endif // KRUEGER_PLATFORM_GRAPHICS_WIN32_C
