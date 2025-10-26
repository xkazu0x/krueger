#ifndef KRUEGER_PLATFORM_GFX_H
#define KRUEGER_PLATFORM_GFX_H

#define BITS_PER_PIXEL 32

typedef u32 Platform_Event_Type;
enum {
  PLATFORM_EVENT_QUIT,
  PLATFORM_EVENT_KEY_PRESS,
  PLATFORM_EVENT_KEY_RELEASE,
};

typedef struct {
  Platform_Event_Type type;
} Platform_Event_Quit;

typedef struct {
  Platform_Event_Type type;
  Keycode keycode;
} Platform_Event_Key;

typedef union {
  Platform_Event_Type type;
  Platform_Event_Quit quit;
  Platform_Event_Key key;
} Platform_Event;

arr_def(Platform_Event)

typedef struct {
  char *window_title;
  s32 window_w;
  s32 window_h;
  s32 back_buffer_w;
  s32 back_buffer_h;
} Platform_Window_Desc;

/////////////////////////
// NOTE: Global Variables

global Platform_Event_Array platform_events;

//////////////////////////////////
// NOTE: Helpers, Implemented Once

internal void platform_gfx_init(void);

/////////////////////////////////
// NOTE: Implemented Per-Platform

internal void platform_create_window(Platform_Window_Desc *desc);
internal void platform_destroy_window(void);
internal void platform_display_back_buffer(u32 *buffer, s32 buffer_w, s32 buffer_h);
internal void platform_update_window_events(void);

#endif // KRUEGER_PLATFORM_GFX_H
