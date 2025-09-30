#ifndef PLATFORM_LINUX_C
#define PLATFORM_LINUX_C

#include <X11/Xlib.h>
#include <X11/Xutil.h>

global Display *display;
global Window window;
global Atom wm_delete_window;
global XImage *image;

internal void
platform_create_window(const char *title, u32 width, u32 height) {
    display = XOpenDisplay(0);
    window = XCreateSimpleWindow(display, XDefaultRootWindow(display), 0, 0, width, height, 0, 0, 0);

    wm_delete_window = XInternAtom(display, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(display, window, &wm_delete_window, 1);

    u32 event_masks = StructureNotifyMask;
    XSelectInput(display, window, event_masks);

    XStoreName(display, window, title);
    XMapWindow(display, window);
    XFlush(display);
}

internal void
platform_destroy_window(void) {
    XUnmapWindow(display, window);
    XDestroyWindow(display, window);
    XCloseDisplay(display);
}

internal void
platform_create_window_buffer(u32 *buffer, u32 width, u32 height) {
    XWindowAttributes attributes = {0};
    XGetWindowAttributes(display, window, &attributes);
    image = XCreateImage(display, attributes.visual, attributes.depth, ZPixmap, 0, 
                         (char *)buffer, width, height, 32, width*sizeof(u32));
}

internal void
platform_display_window_buffer(u32 width, u32 height) {
    GC gc = XCreateGC(display, window, 0, 0);
    XPutImage(display, window, gc, image, 0, 0, 0, 0, width, height);
    XFreeGC(display, gc);
}

internal void
platform_destroy_window_buffer(void) {
    XDestroyImage(image);
}

internal void
platform_update_window_events(void) {
    buf_clear(event_buf);
    while (XPending(display)) {
        XEvent base_event = {0};
        XNextEvent(display, &base_event);
        switch (base_event.type) {
            case ClientMessage: {
                XClientMessageEvent *event = (XClientMessageEvent *)&base_event;
                if ((Atom)event->data.l[0] == wm_delete_window) {
                    Event push_event = {
                        .type = EVENT_QUIT,
                    };
                    buf_push(event_buf, push_event);
                }
            } break;
            case ConfigureNotify: {
                XConfigureEvent *event = (XConfigureEvent *)&base_event;
                Event push_event = {
                    .type = EVENT_WINDOW_RESIZED,
                    .width = event->width,
                    .height = event->height,
                };
                buf_push(event_buf, push_event);
            } break;
        }
    }
}

#endif // PLATFORM_LINUX_C
