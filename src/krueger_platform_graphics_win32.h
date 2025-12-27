#ifndef KRUEGER_PLATFORM_GRAPHICS_WIN32_H
#define KRUEGER_PLATFORM_GRAPHICS_WIN32_H

/////////////////////////////
// NOTE: Includes / Libraries

#pragma comment(lib, "user32")
#pragma comment(lib, "gdi32")

//////////////
// NOTE: Types

typedef struct _Win32_Window _Win32_Window;
struct _Win32_Window {
  _Win32_Window *next;
  _Win32_Window *prev;
  HWND hwnd;
  HDC hdc;
  WINDOWPLACEMENT last_placement;
};

typedef struct {
  Arena *arena;
  HINSTANCE instance;
  ATOM atom;
  Platform_Graphics_Info gfx_info;
  _Win32_Window *first_window;
  _Win32_Window *last_window;
  _Win32_Window *free_window;
  Keycode key_table[0xFF];
} _Win32_Graphics_State;

////////////////////////
// NOTE: Windows Globals

global _Win32_Graphics_State *_win32_gfx_state;
global Platform_Event_List _win32_event_list;
global Arena *_win32_event_arena;

//////////////////////////
// NOTE: Windows Functions

internal Platform_Handle  _win32_handle_from_window(_Win32_Window *window);
internal _Win32_Window     *_win32_window_from_handle(Platform_Handle handle);
internal _Win32_Window     *_win32_window_from_hwnd(HWND hwnd);
internal _Win32_Window     *_win32_window_alloc(void);
internal void             _win32_window_release(_Win32_Window *window);
internal Platform_Event   *_win32_push_event(Platform_Event_Type type, _Win32_Window *window);
internal LRESULT CALLBACK _win32_window_proc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);

#endif // KRUEGER_PLATFORM_GRAPHICS_WIN32_H

