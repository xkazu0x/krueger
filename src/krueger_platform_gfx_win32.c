#ifndef KRUEGER_PLATFORM_GFX_WIN32_C
#define KRUEGER_PLATFORM_GFX_WIN32_C

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

internal void
platform_create_window(Platform_Window_Desc *desc) {
  char *window_title = desc->window_title;

  s32 window_w = desc->window_w;
  s32 window_h = desc->window_h;

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

  HINSTANCE window_instance = GetModuleHandleA(0);
  WNDCLASSA window_class = {
    .style = CS_HREDRAW | CS_VREDRAW,
    .lpfnWndProc = win32_window_proc,
    .cbClsExtra = 0,
    .cbWndExtra = 0,
    .hInstance = window_instance,
    .hIcon = LoadIcon(0, IDI_APPLICATION),
    .hCursor = LoadCursor(0, IDC_ARROW),
    .hbrBackground = CreateSolidBrush(RGB(0, 0, 0)),
    .lpszMenuName = 0,
    .lpszClassName = "krueger_window_class",
  };

  ATOM window_atom = RegisterClassA(&window_class);
  HWND window = CreateWindowExA(window_style_ex, MAKEINTATOM(window_atom),
                                window_title, window_style,
                                window_x, window_y,
                                window_w, window_h,
                                0, 0, window_instance, 0);
  ShowWindow(window, SW_SHOW);
  
  s32 back_buffer_w = desc->back_buffer_w;
  s32 back_buffer_h = desc->back_buffer_h;
  
  BITMAPINFO bitmap_info = {
    .bmiHeader.biSize = sizeof(BITMAPINFOHEADER),
    .bmiHeader.biWidth = back_buffer_w,
    .bmiHeader.biHeight = -back_buffer_h,
    .bmiHeader.biPlanes = 1,
    .bmiHeader.biBitCount = BITS_PER_PIXEL,
    .bmiHeader.biCompression = BI_RGB,
  };

  win32_gfx_state.window = window;
  win32_gfx_state.bitmap_info = bitmap_info;
}

internal void
platform_destroy_window(void) {
  DestroyWindow(win32_gfx_state.window);
}

internal void
platform_display_back_buffer(u32 *buffer, s32 buffer_w, s32 buffer_h) {
  HWND window = win32_gfx_state.window; 
  BITMAPINFO *bitmap_info = &win32_gfx_state.bitmap_info;

  RECT client_rectangle;
  GetClientRect(window, &client_rectangle);
  s32 window_w = client_rectangle.right - client_rectangle.left;
  s32 window_h = client_rectangle.bottom - client_rectangle.top;

  HDC window_device = GetDC(window);
  StretchDIBits(window_device,
                0, 0, window_w, window_h,
                0, 0, buffer_w, buffer_h,
                buffer, bitmap_info,
                DIB_RGB_COLORS, SRCCOPY);
  ReleaseDC(window, window_device);
}

internal void
platform_update_window_events(void) {
  arr_clear(&platform_events);
  MSG message;
  while (PeekMessageA(&message, 0, 0, 0, PM_REMOVE)) {
    switch (message.message) {
      case WM_QUIT: {
        Platform_Event_Quit quit_event = {
          .type = PLATFORM_EVENT_QUIT,
        };
        Platform_Event push_event = {
          .type = quit_event.type,
          .quit = quit_event,
        };
        arr_push(&platform_events, push_event);
      } break;
      case WM_SYSKEYDOWN:
      case WM_SYSKEYUP:
      case WM_KEYDOWN:
      case WM_KEYUP: {
        b32 is_down = ((message.lParam&(1<<31)) == 0);
        Keycode keycode = win32_translate_keycode((u32)message.wParam);
        Platform_Event_Key key_event = {
          .type = (is_down) ? PLATFORM_EVENT_KEY_PRESS : PLATFORM_EVENT_KEY_RELEASE,
          .keycode = keycode,
        };
        Platform_Event push_event = {
          .type = key_event.type,
          .key = key_event,
        };
        arr_push(&platform_events, push_event);
        TranslateMessage(&message);
        DispatchMessageA(&message);
      } break;
      default: {
        TranslateMessage(&message);
        DispatchMessageA(&message);
      }
    }
  }
}

#endif // KRUEGER_PLATFORM_GFX_WIN32_C
