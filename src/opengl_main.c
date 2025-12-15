#define BUILD_CONSOLE_INTERFACE 1
#define PLATFORM_FEATURE_GRAPHICS 1

#include "krueger_base.h"
#include "krueger_platform.h"
#include "krueger_image.h"

#include "krueger_base.c"
#include "krueger_platform.c"
#include "krueger_image.c"

#include <gl/gl.h>
#pragma comment(lib, "opengl32")

internal void
entry_point(int argc, char **argv) {
  String8 window_name = str8_lit("krueger");
  u32 window_w = 800;
  u32 window_h = 600;

  Platform_Handle handle = platform_window_open(window_name, window_w, window_h);
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

  Image image = image_alloc(window_w, window_h);
  u32 texture_handle = 0;
  glGenTextures(1, &texture_handle);
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

    u32 *row = image.pixels;
    for (u32 y = 0; y < image.height; ++y) {
      u32 *pixels = row;
      for (u32 x = 0; x < image.width; ++x) {
        u32 color = ((y%255) << 8) | ((x%255) << 0);
        *pixels++ = color;
      }
      row += image.pitch;
    }

    glViewport(0, 0, window_w, window_h);

    glBindTexture(GL_TEXTURE_2D, texture_handle);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image.width, image.height, 0,
                 GL_BGRA_EXT, GL_UNSIGNED_BYTE, image.pixels);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glEnable(GL_TEXTURE_2D);

    glClearColor(0.2f, 0.3f, 0.3f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    f32 p = 0.8f;
    glBegin(GL_TRIANGLES); {
      glTexCoord2f(0.0f, 0.0f);
      glVertex2f(-p, -p);
      glTexCoord2f(1.0f, 0.0f);
      glVertex2f(p, -p);
      glTexCoord2f(1.0f, 1.0f);
      glVertex2f(p, p);

      glTexCoord2f(0.0f, 0.0f);
      glVertex2f(-p, -p);
      glTexCoord2f(1.0f, 1.0f);
      glVertex2f(p, p);
      glTexCoord2f(0.0f, 1.0f);
      glVertex2f(-p, p);
    } glEnd();

    SwapBuffers(window->hdc);
    scratch_end(scratch);
  }
}
