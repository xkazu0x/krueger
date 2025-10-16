#ifndef KRUEGER_PLATFORM_GFX_WIN32_H
#define KRUEGER_PLATFORM_GFX_WIN32_H

#pragma comment(lib, "user32")
#pragma comment(lib, "gdi32")

typedef struct {
  HWND window;
  BITMAPINFO bitmap_info;
} Win32_Gfx_State;

///////////////////////////////////
// NOTE: Win32 Gfx Global Variables

global Win32_Gfx_State win32_gfx_state;

////////////////////////////
// NOTE: Win32 Gfx Functions

internal LRESULT CALLBACK win32_window_proc(HWND window, UINT message, WPARAM wparam, LPARAM lparam);
internal Keycode win32_translate_keycode(u32 keycode);

#endif // KRUEGER_PLATFORM_GFX_WIN32_H
