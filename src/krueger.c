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

        u32 *out = frame_buffer.pixels;
        for (u32 y = 0; y < frame_buffer.height; ++y) {
            for (u32 x = 0; x < frame_buffer.width; ++x) {
                *out++ = 0x202020;
            }
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
