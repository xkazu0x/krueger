#ifndef PLATFORM_H
#define PLATFORM_H

typedef u32 Event_Type;
enum {
    EVENT_QUIT,
    EVENT_WINDOW_RESIZED,
};

typedef struct {
    Event_Type type;
    u32 width;
    u32 height;
} Event;

global Event *event_buf;

internal void platform_create_window(const char *title, u32 width, u32 height);
internal void platform_destroy_window(void);

internal void platform_create_window_buffer(u32 *buffer, u32 width, u32 height);
internal void platform_display_window_buffer(u32 width, u32 height);
internal void platform_destroy_window_buffer(void);

internal void platform_update_window_events(void);

#endif // PLATFORM_H
