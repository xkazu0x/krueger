#include "base.h"
#include "base.c"

#include <X11/Xlib.h>
#include <GL/glx.h>

#include <stdio.h>

global s32 glx_attributes[] = {
  GLX_RGBA, 
  GLX_RED_SIZE, 1,
  GLX_GREEN_SIZE, 1,
  GLX_BLUE_SIZE, 1,
  GLX_DEPTH_SIZE, 16,
  GLX_DOUBLEBUFFER,
  0,
};

int
main(void) {
  Display *display = XOpenDisplay(0);
  if (display) {
    s32 screen = XDefaultScreen(display);
    Window root = XDefaultRootWindow(display);
    if (glXQueryExtension(display, 0, 0)) {
      XVisualInfo *visual_info = glXChooseVisual(display, screen, glx_attributes);
      if (visual_info) {
        s32 window_x = 0;
        s32 window_y = 0;
        s32 window_width = 800;
        s32 window_height = 600;

        s32 border_width = 0;
        s32 window_depth = visual_info->depth;
        s32 window_class = InputOutput;
        Visual *window_visual = visual_info->visual;

        s32 attribute_mask = CWBackPixel | CWEventMask | CWColormap;

        XSetWindowAttributes window_attributes = {
          .background_pixel = 0xFF00FF,
          .event_mask = StructureNotifyMask,
          .colormap = XCreateColormap(display, root, visual_info->visual, AllocNone),
        };

        Window window = XCreateWindow(display, root, 
                                      window_x, window_y, window_width, window_height,
                                      border_width, window_depth, window_class, window_visual,
                                      attribute_mask, &window_attributes);
        XStoreName(display, window, "krueger");
        XMapWindow(display, window); 

        GLXContext glx_context = glXCreateContext(display, visual_info, 0, true);
        glXMakeCurrent(display, window, glx_context);

        Atom wm_delete_window = XInternAtom(display, "WM_DELETE_WINDOW", false);
        XSetWMProtocols(display, window, &wm_delete_window, 1);

        for (b32 quit = false; !quit;) {
          while (XPending(display)) {
            XEvent base_event = {0};
            XNextEvent(display, &base_event);
            switch (base_event.type) {
              case ClientMessage: {
                XClientMessageEvent *event = (XClientMessageEvent *)&base_event;
                if ((Atom)event->data.l[0] == wm_delete_window) {
                  quit = true;
                }
              } break;
              case ConfigureNotify: {
                XConfigureEvent *event = (XConfigureEvent *)&base_event;
                window_width = event->width;
                window_height = event->height;
              } break;
            }
          }

          glViewport(0, 0, window_width, window_height);

          glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
          glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

          f32 offset = 0.5f;

          glBegin(GL_TRIANGLES); {
            glColor3f(1.0f, 0.0f, 0.0f);
            glVertex2f(0.0f, offset);

            glColor3f(0.0f, 1.0f, 0.0f);
            glVertex2f(offset, -offset);

            glColor3f(0.0f, 0.0f, 1.0f);
            glVertex2f(-offset, -offset);
          } glEnd();

          glXSwapBuffers(display, window); 
        }

        glXMakeCurrent(display, 0, 0);
        glXDestroyContext(display, glx_context);
        XDestroyWindow(display, window);
      }
    }
    XCloseDisplay(display);
  }
  return(0);
}
