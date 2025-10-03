#ifndef PLATFORM_H
#define PLATFORM_H

typedef u32 Keycode;
enum {
  KEY_NULL,

  KEY_0,
  KEY_1,
  KEY_2,
  KEY_3,
  KEY_4,
  KEY_5,
  KEY_6,
  KEY_7,
  KEY_8,
  KEY_9,

  KEY_A,
  KEY_B,
  KEY_C,
  KEY_D,
  KEY_E,
  KEY_F,
  KEY_G,
  KEY_H,
  KEY_I,
  KEY_J,
  KEY_K,
  KEY_L,
  KEY_M,
  KEY_N,
  KEY_O,
  KEY_P,
  KEY_Q,
  KEY_R,
  KEY_S,
  KEY_T,
  KEY_U,
  KEY_V,
  KEY_W,
  KEY_X,
  KEY_Y,
  KEY_Z,

  KEY_UP,
  KEY_LEFT,
  KEY_DOWN,
  KEY_RIGHT,

  KEY_MAX,
};

typedef u32 Event_Type;
enum {
    EVENT_QUIT,
    EVENT_RESIZE,
    EVENT_KEY_PRESS,
    EVENT_KEY_RELEASE,
};

typedef struct {
    Event_Type type;
    Keycode keycode;
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
