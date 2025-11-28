#ifndef KRUEGER_PLATFORM_GRAPHICS_WIN32_H
#define KRUEGER_PLATFORM_GRAPHICS_WIN32_H

/////////////////////////////
// NOTE: Includes / Libraries

#pragma comment(lib, "user32")
#pragma comment(lib, "gdi32")

//////////////
// NOTE: Types

typedef struct Win32_Window Win32_Window;
struct Win32_Window {
  Win32_Window *next;
  Win32_Window *prev;
  HWND hwnd;
  HDC hdc;
  WINDOWPLACEMENT last_placement;
};

typedef struct {
  Arena *arena;
  HINSTANCE instance;
  ATOM atom;
  Platform_Graphics_Info graphics_info;
  Win32_Window *first_window;
  Win32_Window *last_window;
  Win32_Window *free_window;
  Keycode key_table[0xFF];
} Win32_Graphics_State;

////////////////////////
// NOTE: Windows Globals

global Win32_Graphics_State *win32_graphics_state;
global Platform_Event_List win32_event_list;
global Arena *win32_event_arena;

//////////////////////////
// NOTE: Windows Functions

internal Platform_Handle  win32_handle_from_window(Win32_Window *window);
internal Win32_Window *   win32_window_from_handle(Platform_Handle handle);
internal Win32_Window *   win32_window_from_hwnd(HWND hwnd);
internal Win32_Window *   win32_window_alloc(void);
internal void             win32_window_release(Win32_Window *window);
internal Platform_Event * win32_push_event(Platform_Event_Type type, Win32_Window *window);
internal LRESULT CALLBACK win32_window_proc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);

#endif // KRUEGER_PLATFORM_GRAPHICS_WIN32_H
