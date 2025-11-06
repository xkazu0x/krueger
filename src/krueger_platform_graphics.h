#ifndef KRUEGER_PLATFORM_GRAPHICS_H
#define KRUEGER_PLATFORM_GRAPHICS_H

//////////////
// NOTE: Types

typedef struct {
  f32 refresh_rate;
} Platform_Graphics_Info;

typedef enum {
  PLATFORM_EVENT_WINDOW_CLOSE,
  PLATFORM_EVENT_KEY_PRESS,
  PLATFORM_EVENT_KEY_RELEASE,
} Platform_Event_Type;

typedef struct Platform_Event Platform_Event;
struct Platform_Event {
  Platform_Event *next;
  Platform_Event_Type type;
  Platform_Handle window;
  Keycode keycode;
};

typedef struct {
  Platform_Event *first;
  Platform_Event *last;
} Platform_Event_List;

/////////////////////////////////
// NOTE: Implemented Per-Platform

internal void                   platform_graphics_init(void);
internal Platform_Graphics_Info platform_get_graphics_info(void);

internal Platform_Handle        platform_window_open(String8 title, s32 width, s32 height);
internal void                   platform_window_close(Platform_Handle handle);
internal void                   platform_window_show(Platform_Handle handle);
internal void                   platform_window_display_buffer(Platform_Handle handle, u32 *buffer, s32 buffer_w, s32 buffer_h);
internal b32                    platform_window_is_fullscreen(Platform_Handle handle);
internal void                   platform_window_toggle_fullscreen(Platform_Handle handle);

internal Platform_Event_List    platform_get_event_list(Arena *arena);

#endif // KRUEGER_PLATFORM_GRAPHICS_H
