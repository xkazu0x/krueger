#define PLATFORM_FEATURE_GRAPHICS 1

#define OGL_MAJOR_VER 3
#define OGL_MINOR_VER 0

#include "krueger_base.h"
#include "krueger_platform.h"
#include "krueger_opengl.h"

#include "krueger_base.c"
#include "krueger_platform.c"
#include "krueger_opengl.c"

internal void
entry_point(int argc, char **arv) {
  Platform_Handle window = platform_window_open(str8_lit("krueger"), 800, 600);
  platform_window_show(window);

  ogl_window_equip(window);
  ogl_window_select(window);

  for (b32 quit = false; !quit;) {
    Temp scratch = scratch_begin(0, 0);
    Platform_Event_List event_list = platform_get_event_list(scratch.arena);
    for (each_node(Platform_Event, event, event_list.first)) {
      switch (event->type) {
        case PLATFORM_EVENT_WINDOW_CLOSE: {
          quit = true; break;
        } break;
        case PLATFORM_EVENT_KEY_PRESS: {
          if (event->keycode == KEY_Q) {
            quit = true; break;
          }
        } break;
      }
    }
    scratch_end(scratch);
    if (quit) break;

    Rect2 window_rect = platform_get_window_client_rect(window);
    Vector2 window_size = vector2_sub(window_rect.max, window_rect.min);
    glViewport(0, 0, (u32)window_size.x, (u32)window_size.y);

    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    f32 offset = 0.9f;
    glBegin(GL_TRIANGLES); {
      glColor3f(1.0f, 0.5f, 0.3f);
      glVertex2f(0.0f, offset);

      glColor3f(1.0f, 0.5f, 0.3f);
      glVertex2f(offset, -offset);

      glColor3f(1.0f, 0.5f, 0.3f);
      glVertex2f(-offset, -offset);
    } glEnd();

    ogl_window_swap(window);
  }

  platform_window_close(window);
}
