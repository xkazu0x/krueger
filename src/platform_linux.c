#ifndef PLATFORM_LINUX_C
#define PLATFORM_LINUX_C

#include <X11/Xlib.h>
#include <X11/Xutil.h>

typedef struct {
    Display *display;
    Window window;
    Atom wm_delete_window;
    XImage *image;
} Platform_Linux_State;

global Platform_Linux_State linux_state;

internal void
platform_create_window(const char *title, u32 width, u32 height) {
    linux_state.display = XOpenDisplay(0);
    linux_state.window = XCreateSimpleWindow(linux_state.display, XDefaultRootWindow(linux_state.display), 0, 0, width, height, 0, 0, 0);

    linux_state.wm_delete_window = XInternAtom(linux_state.display, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(linux_state.display, linux_state.window, &linux_state.wm_delete_window, 1);

    u32 event_masks = StructureNotifyMask;
    XSelectInput(linux_state.display, linux_state.window, event_masks);

    XStoreName(linux_state.display, linux_state.window, title);
    XMapWindow(linux_state.display, linux_state.window);
    XFlush(linux_state.display);
}

internal void
platform_destroy_window(void) {
    XUnmapWindow(linux_state.display, linux_state.window);
    XDestroyWindow(linux_state.display, linux_state.window);
    XCloseDisplay(linux_state.display);
}

internal void
platform_create_window_buffer(u32 *buffer, u32 width, u32 height) {
    XWindowAttributes attributes = {0};
    XGetWindowAttributes(linux_state.display, linux_state.window, &attributes);
    linux_state.image = XCreateImage(linux_state.display, attributes.visual, attributes.depth, ZPixmap, 0, 
                                     (char *)buffer, width, height, 32, width*sizeof(u32));
}

internal void
platform_display_window_buffer(u32 width, u32 height) {
    GC gc = XCreateGC(linux_state.display, linux_state.window, 0, 0);
    XPutImage(linux_state.display, linux_state.window, gc, linux_state.image, 0, 0, 0, 0, width, height);
    XFreeGC(linux_state.display, gc);
}

internal void
platform_destroy_window_buffer(void) {
    XDestroyImage(linux_state.image);
}

internal void
platform_update_window_events(void) {
    buf_clear(event_buf);
    while (XPending(linux_state.display)) {
        XEvent base_event = {0};
        XNextEvent(linux_state.display, &base_event);
        switch (base_event.type) {
            case ClientMessage: {
                XClientMessageEvent *event = (XClientMessageEvent *)&base_event;
                if ((Atom)event->data.l[0] == linux_state.wm_delete_window) {
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
