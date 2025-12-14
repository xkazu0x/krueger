#define BUILD_ENTRY_POINT 0

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NO_MIN_MAX
#define NO_MIN_MAX
#endif
#ifndef UNICODE
#define UNICODE
#endif
#include <windows.h>
#pragma comment(lib, "user32")
#pragma comment(lib, "gdi32")

#include "krueger_base.h"
#include "krueger_platform.h"

#include "krueger_base.c"
#include "krueger_platform.c"

//////////////
// NOTE: Types

typedef struct String16 String16;
struct String16 {
  uxx len;
  u16 *str;
};

typedef struct String32 String32;
struct String32 {
  uxx len;
  u32 *str;
};

typedef struct {
  u32 inc;
  u32 codepoint;
} Unicode_Decode;

internal String16
str16(u16 *str, uxx len) {
  String16 result = {
    .len = len,
    .str = str,
  };
  return(result);
}

internal String32
str32(u32 *str, uxx len) {
  String32 result = {
    .len = len,
    .str = str,
  };
  return(result);
}

global const u8 utf8_class[32] = {
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,2,2,2,2,3,3,4,5,
};

internal Unicode_Decode
utf8_decode(u8 *str, uxx max) {
  Unicode_Decode result = {1, u32_max};
  u8 byte = str[0];
  u8 byte_class = utf8_class[byte >> 3];
  switch (byte_class) {
    case 1: {
      result.codepoint = byte;
    } break;
    case 2: {
      if (1 < max) {
        u8 count_byte = str[1];
        if (utf8_class[count_byte >> 3] == 0) {
          result.codepoint = ((byte & bitmask5) << 6);
          result.codepoint |= (count_byte & bitmask6);
          result.inc = 2;
        }
      }
    } break;
    case 3: {
      if (2 < max) {
        u8 count_byte[2] = {str[1], str[2]};
        if (utf8_class[count_byte[0] >> 3] == 0 &&
          utf8_class[count_byte[1] >> 3] == 0) {
          result.codepoint = ((byte & bitmask4) << 12);
          result.codepoint |= ((count_byte[0] & bitmask6) << 6);
          result.codepoint |= (count_byte[1] & bitmask6);
          result.inc = 3;
        }
      }
    } break;
    case 4: {
      if (3 < max) {
        u8 count_byte[3] = {str[1], str[2], str[3]};
        if (utf8_class[count_byte[0] >> 3] == 0 &&
          utf8_class[count_byte[1] >> 3] == 0 &&
          utf8_class[count_byte[2] >> 3] == 0) {
          result.codepoint = ((byte & bitmask3) << 18);
          result.codepoint |= ((count_byte[0] & bitmask6) << 12);
          result.codepoint |= ((count_byte[1] & bitmask6) << 6);
          result.codepoint |= (count_byte[2] & bitmask6);
          result.inc = 4;
        }
      }
    } break;
  }
  return(result);
}

internal u32
utf8_encode(u8 *str, u32 codepoint) {
  u32 inc = 0;
  if (codepoint <= 0x7F) {
    str[0] = (u8)codepoint;
    inc = 1;
  } else if (codepoint <= 0x7FF) {
    str[0] = (u8)((bitmask2 << 6) | ((codepoint >> 6) & bitmask5));
    str[1] = (u8)(bit8 | (codepoint & bitmask6));
    inc = 2;
  } else if (codepoint <= 0xFFFF) {
    str[0] = (u8)((bitmask3 << 5) | ((codepoint >> 12) & bitmask4));
    str[1] = (u8)(bit8 | ((codepoint >> 6) & bitmask6));
    str[2] = (u8)(bit8 | (codepoint & bitmask6));
    inc = 3;
  } else if (codepoint <= 0x10FFFF) {
    str[0] = (u8)((bitmask4 << 4) | ((codepoint >> 18) & bitmask3));
    str[1] = (u8)(bit8 | ((codepoint >> 12) & bitmask6));
    str[2] = (u8)(bit8 | ((codepoint >> 6) & bitmask6));
    str[3] = (u8)(bit8 | (codepoint & bitmask6));
    inc = 4;
  } else {
    str[0] = '?';
    inc = 1;
  }
  return(inc);
}

internal Unicode_Decode
utf16_decode(u16 *str, uxx max) {
  Unicode_Decode result = {1, u32_max};
  result.codepoint = str[0];
  result.inc = 1;
  if ((max > 1) &&
      (0xD800 <= str[0] && str[0] <= 0xDBFF) &&
      (0xDC00 <= str[1] && str[1] <= 0xDFFF)) {
    result.codepoint = ((str[0] - 0xD800) << 10) | ((str[1] - 0xDC00) + 0x10000);
    result.inc = 2;
  }
  return(result);
}

internal u32
utf16_encode(u16 *str, u32 codepoint) {
  u32 inc = 0;
  if (codepoint == u32_max) {
    str[0] = '?';
    inc = 1;
  } else if (codepoint < 0x10000) {
    str[0] = (u16)codepoint;
    inc = 1;
  } else {
    u32 u = codepoint - 0x10000;
    str[0] = (u16)(0xD800 + (u >> 10));
    str[1] = (u16)(0xDC00 + (u & bitmask10));
    inc = 2;
  }
  return(inc);
}

internal String16
str16_from_str8(Arena *arena, String8 in) {
  String16 result = {0};
  if (in.len) {
    uxx cap = in.len;
    u16 *str = push_array(arena, u16, cap + 1);
    u8 *ptr = in.str;
    u8 *opl = ptr + in.len;
    uxx len = 0;
    Unicode_Decode consume;
    for (;ptr < opl; ptr += consume.inc) {
      consume = utf8_decode(ptr, opl - ptr);
      len += utf16_encode(str + len, consume.codepoint);
    }
    str[len] = 0;
    arena->cmt_size -= (cap - len)*sizeof(u16);
    result = str16(str, len);
  }
  return(result);
}

internal String32
str32_from_str8(Arena *arena, String8 in) {
  String32 result = {0};
  if (in.len) {
    uxx cap = in.len;
    u32 *str = push_array(arena, u32, cap + 1);
    u8 *ptr = in.str;
    u8 *opl = ptr + in.len;
    uxx len = 0;
    Unicode_Decode consume;
    for (;ptr < opl; ptr += consume.inc) {
      consume = utf8_decode(ptr, opl - ptr);
      str[len] = consume.codepoint;
      len += 1;
    }
    str[len] = 0;
    arena->cmt_size -= (cap - len)*sizeof(u32);
    result = str32(str, len);
  }
  return(result);
}

internal String8
str8_from_str16(Arena *arena, String16 in) {
  String8 result = {0};
  if (in.len) {
    uxx cap = in.len*sizeof(u16);
    u8 *str = push_array(arena, u8, cap + 1);
    u16 *ptr = in.str;
    u16 *opl = ptr + in.len;
    uxx len = 0;
    Unicode_Decode consume;
    for (;ptr < opl; ptr += consume.inc) {
      consume = utf16_decode(ptr, opl - ptr);
      len += utf8_encode(str + len, consume.codepoint);
    }
    str[len] = 0;
    arena->cmt_size -= (cap - len);
    result = str8(str, len);
  }
  return(result);
}

internal String8
str8_from_str32(Arena *arena, String32 in) {
  String8 result = {0};
  if (in.len) {
    uxx cap = in.len*sizeof(u32);
    u8 *str = push_array(arena, u8, cap + 1);
    u32 *ptr = in.str;
    u32 *opl = ptr + in.len;
    uxx len = 0;
    for (;ptr < opl; ptr += 1) {
      len += utf8_encode(str + len, *ptr);
    }
    str[len] = 0;
    arena->cmt_size -= (cap - len);
    result = str8(str, len);
  }
  return(result);
}

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

int
wmain(int argc, char **argv) {
  utf8_test();
  str32_conversion_test();
  utf16_test();
  str16_conversion_test();

  Thread_Context *thread_context = thread_context_alloc();
  thread_context_select(thread_context);

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

  HINSTANCE instance = GetModuleHandle(0);
  WNDCLASSEXW window_class = { .cbSize = sizeof(window_class) };
  window_class.style = CS_VREDRAW | CS_HREDRAW;
  window_class.lpfnWndProc = win32_window_procx;
  window_class.hInstance = instance;
  window_class.hIcon = LoadIcon(0, IDI_APPLICATION);
  window_class.hCursor = LoadCursor(0, IDC_ARROW);
  window_class.hbrBackground = CreateSolidBrush(RGB(0, 0, 0));
  window_class.lpszClassName = L"krueger_graphical_windows";
  ATOM atom = RegisterClassEx(&window_class);

  HWND hwnd = CreateWindowEx(window_style_ex, MAKEINTATOM(atom),
                             window_name16.str, window_style,
                             window_x, window_y, window_w, window_h,
                             0, 0, instance, 0);
  (void)hwnd;

  for (b32 quit = false; !quit;) {
    MSG message;
    while (PeekMessage(&message, 0, 0, 0, PM_REMOVE)) {
      if (message.message == WM_QUIT) {
        quit = true;
        break;
      }
      TranslateMessage(&message);
      DispatchMessage(&message);
    }
  }

  scratch_end(scratch);
}
