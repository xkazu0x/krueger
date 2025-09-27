#include "base.h"

#include <stdio.h>
#include <stdlib.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>

typedef struct {
    u32 width;
    u32 height;
    u32 *pixels;
} Image;

internal Image
alloc_image(u32 width, u32 height) {
    Image result = {
        .width = width,
        .height = height,
        .pixels = malloc(width*height*sizeof(u32)),
    };
    return(result);
}

internal void
image_clear(Image image, u32 color) {
    u32 *out = image.pixels;
    for (u32 y = 0; y < image.height; ++y) {
        for (u32 x = 0; x < image.width; ++x) {
            *out++ = color;
        }
    }
}

internal void
draw_line(u32 *buffer, u32 width, u32 height,
          s32 x0, s32 y0, s32 x1, s32 y1,
          u32 color) {
    if (abs_t(s32, x1 - x0) > abs_t(s32, y1 - y0)) {
        if (x0 > x1) {
            swap_t(s32, x0, x1);
            swap_t(s32, y0, y1);
        }
        s32 dx = x1 - x0;
        s32 dy = y1 - y0;
        s32 dir = (dy < 0) ? -1 : 1;
        dy *= dir;
        if (dx != 0) {
            s32 y = y0;
            s32 d = 2*dy - dx;
            for (s32 x = x0; x <= x1; ++x) {
                if ((y >= 0 && y < (s32)height) &&
                    (x >= 0 && x < (s32)width)) {
                    u32 *dst = buffer + y*width + x;
                    *dst = color; 
                }
                if (d >= 0) {
                    y += dir;
                    d = d - 2*dx;
                }
                d = d + 2*dy;
            }
        }
    } else {
        if (y0 > y1) {
            swap_t(s32, x0, x1);
            swap_t(s32, y0, y1);
        }
        s32 dx = x1 - x0;
        s32 dy = y1 - y0;
        s32 dir = (dx < 0) ? -1 : 1;
        dx *= dir;
        if (dy != 0) {
            s32 x = x0;
            s32 d = 2*dx - dy;
            for (s32 y = y0; y <= y1; ++y) {
                if ((y >= 0 && y < (s32)height) &&
                    (x >= 0 && x < (s32)width)) {
                    u32 *dst = buffer + y*width + x;
                    *dst = color; 
                }
                if (d >= 0) {
                    x += dir;
                    d = d - 2*dy;
                }
                d = d + 2*dx;
            }
        }
    }
}

typedef struct {
    f32 x, y;
} Vector2;

internal Vector2
make_vector2(f32 x, f32 y) {
    Vector2 result = { 
        .x = x,
        .y = y,
    };
    return(result);
}

internal Vector2
project_to_screen(Vector2 p, u32 w, u32 h) {
    Vector2 result = {
        .x = (p.x + 1.0f)*(f32)w*0.5f,
        .y = (-p.y + 1.0f)*(f32)h*0.5f,
    };
    return(result);
}

int
main(void) {
    char *window_title = "krueger";
    u32 window_width = 800;
    u32 window_height = 600;

    Image frame_buffer = alloc_image(window_width, window_height);

    Display *display = XOpenDisplay(0);
    Window window = XCreateSimpleWindow(display, XDefaultRootWindow(display), 0, 0, window_width, window_height, 0, 0, 0);

    Atom wm_delete_window = XInternAtom(display, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(display, window, &wm_delete_window, 1);

    u32 event_masks = StructureNotifyMask;
    XSelectInput(display, window, event_masks);

    XStoreName(display, window, window_title);
    XMapWindow(display, window);
    XFlush(display);

    XWindowAttributes attributes = {0};
    XGetWindowAttributes(display, window, &attributes);
    XImage *image = XCreateImage(display, attributes.visual, attributes.depth, ZPixmap, 0, 
                                 (char *)frame_buffer.pixels, frame_buffer.width, frame_buffer.height, 
                                 32, frame_buffer.width*sizeof(u32));

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
                    XDestroyImage(image);
                    frame_buffer = alloc_image(window_width, window_height);
                    image = XCreateImage(display, attributes.visual, attributes.depth, ZPixmap, 0, 
                                         (char *)frame_buffer.pixels, frame_buffer.width, frame_buffer.height, 
                                         32, frame_buffer.width*sizeof(u32));
                } break;
            }
        }
        
        Vector2 points[] = {
            { -0.5f, -0.5f },
            {  0.0f,  0.5f },
            {  0.5f, -0.5f },
        };

        for (u32 point_index = 0;
             point_index < array_count(points);
             ++point_index) {
            Vector2 *point = points + point_index;
            *point = project_to_screen(*point, window_width, window_height);
        }

        image_clear(frame_buffer, 0x202020);
        for (u32 point_index = 0;
             point_index < array_count(points);
             point_index += 3) {
            Vector2 p0 = points[point_index];
            Vector2 p1 = points[point_index + 1];
            Vector2 p2 = points[point_index + 2];
            draw_line(frame_buffer.pixels, frame_buffer.width, frame_buffer.height,
                      p0.x, p0.y, p1.x, p1.y, 0xaa2020);
            draw_line(frame_buffer.pixels, frame_buffer.width, frame_buffer.height,
                      p1.x, p1.y, p2.x, p2.y, 0xaa2020);
            draw_line(frame_buffer.pixels, frame_buffer.width, frame_buffer.height,
                      p2.x, p2.y, p0.x, p0.y, 0xaa2020);
        }

        GC gc = XCreateGC(display, window, 0, 0);
        XPutImage(display, window, gc, image, 0, 0, 0, 0, window_width, window_height);
        XFreeGC(display, gc);
    }

    XUnmapWindow(display, window);
    XDestroyWindow(display, window);
    XCloseDisplay(display);
    return(0);
}
