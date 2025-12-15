#define BUILD_CONSOLE_INTERFACE 1
#define PLATFORM_FEATURE_GRAPHICS 1

#include "krueger_base.h"
#include "krueger_platform.h"

#include "krueger_base.c"
#include "krueger_platform.c"

#include <gl/gl.h>
#pragma comment(lib, "opengl32")

internal void
gl_window_select(Platform_Handle handle) {
}

internal void
gl_window_swap_buffers(Platform_Handle handle) {
  Win32_Window *window = win32_window_from_handle(handle);
  if (window != 0) {
  }
}

internal void
entry_point(int argc, char **argv) {
  Platform_Handle handle = platform_window_open(str8_lit("krueger"), 800, 600);
  Win32_Window *window = win32_window_from_handle(handle);

  PIXELFORMATDESCRIPTOR desired_pixel_format = {.nSize = sizeof(desired_pixel_format)};
  desired_pixel_format.nVersion = 1;
  desired_pixel_format.dwFlags = PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW | PFD_DOUBLEBUFFER;
  desired_pixel_format.iPixelType = PFD_TYPE_RGBA;
  desired_pixel_format.cColorBits = 32;
  desired_pixel_format.cAlphaBits = 8;
  desired_pixel_format.iLayerType = PFD_MAIN_PLANE;

  int suggested_pixel_format_index = ChoosePixelFormat(window->hdc, &desired_pixel_format);
  PIXELFORMATDESCRIPTOR suggested_pixel_format;
  DescribePixelFormat(window->hdc, suggested_pixel_format_index, sizeof(suggested_pixel_format), &suggested_pixel_format);
  SetPixelFormat(window->hdc, suggested_pixel_format_index, &suggested_pixel_format);

  HGLRC glrc = wglCreateContext(window->hdc); 
  if (!wglMakeCurrent(window->hdc, glrc)) {
    invalid_path;
  }

  platform_window_show(handle);
  for (b32 quit = false; !quit;) {
    Temp scratch = scratch_begin(0, 0);
    Platform_Event_List event_list = platform_get_event_list(scratch.arena);
    for (Platform_Event *event = event_list.first; event != 0; event = event->next) {
      switch (event->type) {
        case PLATFORM_EVENT_WINDOW_CLOSE: {
          quit = true;
          break;
        } break;
        case PLATFORM_EVENT_KEY_PRESS: {
          if (event->keycode == KEY_Q) {
            quit = true;
            break;
          }
        } break;
      }
    }
    if (quit) break;

    glClearColor(0.2f, 0.3f, 0.3f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    SwapBuffers(window->hdc);

    scratch_end(scratch);
  }
}
