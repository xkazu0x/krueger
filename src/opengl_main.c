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
gl_window_equip(Platform_Handle handle) {
  if (platform_handle_is_valid(handle)) {
    Win32_Window *window = win32_window_from_handle(handle);

    PIXELFORMATDESCRIPTOR desired_pixel_format = {.nSize = sizeof(desired_pixel_format)};
    desired_pixel_format.nVersion = 1;
    desired_pixel_format.dwFlags = PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW | PFD_DOUBLEBUFFER;
    desired_pixel_format.iPixelType = PFD_TYPE_RGBA;
    desired_pixel_format.cColorBits = 32;
    desired_pixel_format.cDepthBits = 24;
    desired_pixel_format.cStencilBits = 8;
    desired_pixel_format.iLayerType = PFD_MAIN_PLANE;

    int suggested_pixel_format_index = ChoosePixelFormat(window->hdc, &desired_pixel_format);
    PIXELFORMATDESCRIPTOR suggested_pixel_format;
    DescribePixelFormat(window->hdc, suggested_pixel_format_index, sizeof(suggested_pixel_format), &suggested_pixel_format);
    SetPixelFormat(window->hdc, suggested_pixel_format_index, &suggested_pixel_format);

    HGLRC glrc = wglCreateContext(window->hdc); 
    if (!wglMakeCurrent(window->hdc, glrc)) {
      invalid_path;
    }
  }
}

internal void
gl_window_swap(Platform_Handle handle) {
  if (platform_handle_is_valid(handle)) {
    Win32_Window *window = win32_window_from_handle(handle);
    SwapBuffers(window->hdc);
  }
}

typedef struct {
  Vector2 min, max;
} Rect2;

internal Rect2
platform_window_get_client_rect(Platform_Handle handle) {
  Rect2 result = {0};
  if (platform_handle_is_valid(handle)) {
    Win32_Window *window = win32_window_from_handle(handle);
    RECT client_rect;
    GetClientRect(window->hwnd, &client_rect);
    result.min.x = (f32)client_rect.left;
    result.min.y = (f32)client_rect.top;
    result.max.x = (f32)client_rect.right;
    result.max.y = (f32)client_rect.bottom;
  }
  return(result);
}

internal void
render_image_to_window(Platform_Handle window, Image image) {
  Rect2 client_rect = platform_window_get_client_rect(window);
  Vector2 window_size = vector2_sub(client_rect.max, client_rect.min);
  glViewport(0, 0, (s32)window_size.x, (s32)window_size.y);

  glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
  glClear(GL_COLOR_BUFFER_BIT);

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();

  glBindTexture(GL_TEXTURE_2D, 0);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image.width, image.height, 0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, image.pixels);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
  glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
  glEnable(GL_TEXTURE_2D);

  f32 display_w = image.width*(window_size.y/image.height);
  f32 display_h = image.height*(window_size.x/image.width);

  if (window_size.x >= display_w) {
    display_h = window_size.y;
  } else if (window_size.y >= display_h) {
    display_w = window_size.x;
  }

  f32 x = display_w/window_size.x;
  f32 y = display_h/window_size.y;

  glBegin(GL_TRIANGLES); {
    glTexCoord2f(0.0f, 0.0f);
    glVertex2f(-x, -y);
    glTexCoord2f(1.0f, 0.0f);
    glVertex2f(x, -y);
    glTexCoord2f(1.0f, 1.0f);
    glVertex2f(x, y);

    glTexCoord2f(0.0f, 0.0f);
    glVertex2f(-x, -y);
    glTexCoord2f(1.0f, 1.0f);
    glVertex2f(x, y);
    glTexCoord2f(0.0f, 1.0f);
    glVertex2f(-x, y);
  } glEnd();

  gl_window_swap(window);
}

internal void
entry_point(int argc, char **argv) {
  u32 window_scale = 5;
  u32 render_w = 128;
  u32 render_h = 128;

  String8 window_name = str8_lit("krueger");
  u32 window_w = window_scale*render_w;
  u32 window_h = window_scale*render_h;

  Platform_Handle window = platform_window_open(window_name, window_w, window_h);
  gl_window_equip(window);

  Image image = image_alloc(render_w, render_h);
  platform_window_show(window);

  for (b32 quit = false; !quit;) {
    Temp scratch = scratch_begin(0, 0);
    Platform_Event_List event_list = platform_get_event_list(scratch.arena);
    for (Platform_Event *event = event_list.first; event != 0; event = event->next) {
      switch (event->type) {
        case PLATFORM_EVENT_WINDOW_CLOSE: {
          quit = true;
        } break;
        case PLATFORM_EVENT_KEY_PRESS: {
          if (event->keycode == KEY_Q) {
            quit = true;
          }
        } break;
      }
    }

    u32 *row = image.pixels;
    for (u32 y = 0; y < image.height; ++y) {
      u32 *pixels = row;
      for (u32 x = 0; x < image.width; ++x) {
        u32 color = 0x202020;
        if ((x + y) % 2 == 0) {
          color = 0xAAAAAA;
        }
        *pixels++ = color;
      }
      row += image.pitch;
    }

    render_image_to_window(window, image);
    scratch_end(scratch);
  }
}
