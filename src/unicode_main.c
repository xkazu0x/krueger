#define BUILD_CONSOLE_INTERFACE 1
#define PLATFORM_FEATURE_GRAPHICS 1

#include "krueger_base.h"
#include "krueger_platform.h"

#include "krueger_base.c"
#include "krueger_platform.c"

///////////////////////
// NOTE: Test Functions

internal void
utf8_test(void) {
  u8 str[] = {
    0xE3, 0x81, 0x8B, // NOTE: か U+304B
    0xE3, 0x81, 0x9A, // NOTE: ず U+305A
    0xE3, 0x81, 0x8A, // NOTE: お U+304A
  };

  printf("[sample]:\n");
  for (u32 i = 0; i < array_count(str); i += 3) {
    printf("0x%X 0x%X 0x%X\n", str[i+0], str[i+1], str[i+2]);
  }

  printf("[decoded]:\n");
  u8 *ptr = str;
  u8 *opl = str + array_count(str);
  for (;ptr != opl;) {
    Unicode_Decode decode = utf8_decode(ptr, (opl - ptr));
    ptr += decode.inc;
    printf("[%d] U+%X -> ", decode.inc, decode.codepoint);
  
    u8 encoded[4];
    u32 inc = utf8_encode(encoded, decode.codepoint);
    for (u32 i = 0; i < inc; ++i) {
      printf("0x%X ", encoded[i]);
      if (i == inc - 1) printf("\n");
    }
  }
}

internal void
str32_conversion_test(void) {
  Arena *arena = arena_alloc();
  String8 string8 = str8_lit("young vamp life");
  String32 string32 = str32_from_str8(arena, string8);
  uxx string32_size = (string32.len + 1)*sizeof(u32);
  assert(string32.len == string8.len);
  assert(arena->cmt_size == string32_size);
  String8 _string8 = str8_from_str32(arena, string32);
  assert(_string8.len == string8.len);
  assert((arena->cmt_size - string32_size) == (_string8.len + 1));
  for (uxx i = 0; i < _string8.len; ++i) {
    assert(_string8.str[i] == string8.str[i]);
  }
  arena_release(arena);
}

internal void
utf16_test(void) {
  u16 str[2];
  u32 codepoint = 0x10437;
  u32 inc = utf16_encode(str, codepoint);
  assert(inc == 2);
  printf("[%d] ", inc);
  for (u32 i = 0; i < array_count(str); ++i) {
    printf("0x%X ", str[i]);
  }
  printf("-> ");
  u16 *ptr = str;
  u16 *opl = ptr + array_count(str);
  Unicode_Decode consume;
  for (;ptr < opl; ptr += consume.inc) {
    consume = utf16_decode(str, opl - ptr);
    assert(consume.codepoint == codepoint);
    printf("U+%X\n", consume.codepoint);
  }
}

internal void
str16_conversion_test(void) {
  Arena *arena = arena_alloc();
  String8 string8 = str8_lit("young vamp life");
  String16 string16 = str16_from_str8(arena, string8);
  uxx string16_size = (string16.len + 1)*sizeof(u16);
  assert(string16.len == string8.len);
  assert(arena->cmt_size == string16_size);
  String8 _string8 = str8_from_str16(arena, string16);
  assert(_string8.len == string8.len);
  assert((arena->cmt_size - string16_size) == (_string8.len + 1));
  for (uxx i = 0; i < _string8.len; ++i) {
    assert(_string8.str[i] == string8.str[i]);
  }
  arena_release(arena);
}

internal LRESULT
win32_window_procx(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) {
  LRESULT result = 0;
  switch (message) {
    case WM_CLOSE: {
      PostQuitMessage(0);
    } break;
    default: {
      result = DefWindowProcW(hwnd, message, wparam, lparam);
    } break;
  }
  return(result);
}

internal void
entry_point(int argc, char **argv) {
  utf8_test();
  str32_conversion_test();
  utf16_test();
  str16_conversion_test();

  Temp scratch = scratch_begin(0, 0);

  String8 window_name8 = str8_lit("krueger");
  String16 window_name16 = str16_from_str8(scratch.arena, window_name8);

  u32 window_w = 800;
  u32 window_h = 600;

  u32 window_style = WS_OVERLAPPEDWINDOW | WS_VISIBLE;
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

  HINSTANCE instance = GetModuleHandleW(0);
  WNDCLASSEXW window_class = { .cbSize = sizeof(window_class) };
  window_class.style = CS_VREDRAW | CS_HREDRAW;
  window_class.lpfnWndProc = win32_window_procx;
  window_class.hInstance = instance;
  window_class.hIcon = LoadIconW(0, IDI_APPLICATION);
  window_class.hCursor = LoadCursorW(0, IDC_ARROW);
  window_class.hbrBackground = CreateSolidBrush(RGB(0, 0, 0));
  window_class.lpszClassName = L"krueger_graphical_windows";
  ATOM atom = RegisterClassExW(&window_class);

  HWND hwnd = CreateWindowExW(window_style_ex, MAKEINTATOM(atom),
                              window_name16.str, window_style,
                              window_x, window_y, window_w, window_h,
                              0, 0, instance, 0);
  (void)hwnd;

  for (b32 quit = false; !quit;) {
    MSG message;
    while (PeekMessageW(&message, 0, 0, 0, PM_REMOVE)) {
      if (message.message == WM_QUIT) {
        quit = true;
        break;
      }
      TranslateMessage(&message);
      DispatchMessageW(&message);
    }
  }

  scratch_end(scratch);
}
