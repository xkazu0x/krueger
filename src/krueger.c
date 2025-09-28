#include "base.h"

#include <stdio.h>
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

internal void
fill_triangle(u32 *pixels, u32 width, u32 height,
              s32 x0, s32 y0,
              s32 x1, s32 y1,
              s32 x2, s32 y2,
              u32 color) {
    s32 min_x = clamp_bot(0, min(min(x0, x1), x2));
    s32 min_y = clamp_bot(0, min(min(y0, y1), y2));
    s32 max_x = clamp_top(max(max(x0, x1), x2), (s32)width);
    s32 max_y = clamp_top(max(max(y0, y1), y2), (s32)height);
    s32 x01 = x1 - x0;
    s32 y01 = y1 - y0;
    s32 x12 = x2 - x1;
    s32 y12 = y2 - y1;
    s32 x20 = x0 - x2;
    s32 y20 = y0 - y2;
    for (s32 y = min_y; y < max_y; ++y) {
        s32 dy0 = y - y0;
        s32 dy1 = y - y1;
        s32 dy2 = y - y2;
        for (s32 x = min_x; x < max_x; ++x) {
            s32 dx0 = x - x0;
            s32 dx1 = x - x1;
            s32 dx2 = x - x2;
            s32 w0 = x01*dy0 - y01*dx0;
            s32 w1 = x12*dy1 - y12*dx1;
            s32 w2 = x20*dy2 - y20*dx2;
            if ((w0 >= 0) && (w1 >= 0) && (w2 >= 0)) {
                pixels[y*width + x] = color;
            }
        }
    }
}

typedef struct {
    f32 m[4][4];
} Matrix4x4;

internal Matrix4x4
matrix4x4_perspective(f32 fov, f32 aspect_ratio, f32 z_near, f32 z_far) {
    Matrix4x4 result = {0};
    result.m[0][0] = fov*aspect_ratio;
    result.m[1][1] = fov;
    result.m[2][2] = z_far / (z_far - z_near);
    result.m[3][2] = (-z_far*z_near) / (z_far - z_near);
    result.m[2][3] = 1.0f;
    return(result);
}

internal Vector3
matrix4x4_mul(Matrix4x4 m, Vector3 v) {
    Vector3 result = {
        .x = v.x*m.m[0][0] + v.y*m.m[1][0] + v.z*m.m[2][0] + m.m[3][0],
        .y = v.x*m.m[0][1] + v.y*m.m[1][1] + v.z*m.m[2][1] + m.m[3][1],
        .z = v.x*m.m[0][2] + v.y*m.m[1][2] + v.z*m.m[2][2] + m.m[3][2],
    };
    float w = v.x*m.m[0][3] + v.y*m.m[1][3] + v.z*m.m[2][3] + m.m[3][3];
    if (w != 0.0f) {
        result.x /= w;
        result.y /= w;
        result.z /= w;
    }
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

    Vector3 vertices[] = {
        // front-face
        { { -0.5f, -0.5f, 0.0f } },
        { { -0.5f,  0.5f, 0.0f } },
        { {  0.5f, -0.5f, 0.0f } },
        { { -0.5f,  0.5f, 0.0f } },
        { {  0.5f,  0.5f, 0.0f } },
        { {  0.5f, -0.5f, 0.0f } },

        // back-face
        { {  0.5f, -0.5f, 1.0f } },
        { {  0.5f,  0.5f, 1.0f } },
        { { -0.5f, -0.5f, 1.0f } },
        { {  0.5f,  0.5f, 1.0f } },
        { { -0.5f,  0.5f, 1.0f } },
        { { -0.5f, -0.5f, 1.0f } },

        // left-face
        { { -0.5f, -0.5f, 1.0f } },
        { { -0.5f,  0.5f, 1.0f } },
        { { -0.5f, -0.5f, 0.0f } },

        { { -0.5f,  0.5f, 1.0f } },
        { { -0.5f,  0.5f, 0.0f } },
        { { -0.5f, -0.5f, 0.0f } },

        // right-face
        { { 0.5f, -0.5f, 0.0f } },
        { { 0.5f,  0.5f, 0.0f } },
        { { 0.5f, -0.5f, 1.0f } },

        { { 0.5f,  0.5f, 0.0f } },
        { { 0.5f,  0.5f, 1.0f } },
        { { 0.5f, -0.5f, 1.0f } },

        // top-face
        { { -0.5f, 0.5f, 0.0f } },
        { { -0.5f, 0.5f, 1.0f } },
        { {  0.5f, 0.5f, 0.0f } },

        { { -0.5f, 0.5f, 1.0f } },
        { {  0.5f, 0.5f, 1.0f } },
        { {  0.5f, 0.5f, 0.0f } },

        // bottom-face
        { { -0.5f, -0.5f, 1.0f } },
        { { -0.5f, -0.5f, 0.0f } },
        { {  0.5f, -0.5f, 1.0f } },

        { { -0.5f, -0.5f, 0.0f } },
        { {  0.5f, -0.5f, 0.0f } },
        { {  0.5f, -0.5f, 1.0f } },
    };

    Vector3 cam_p = make_vector3(0.0f, 0.0f, 0.0f);

    s32 tick = 0;
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
        
        f32 half_fov = 90.0f*0.5f;
        f32 fovr = 1.0f / tan_f32(radians_f32(half_fov));
        f32 aspect_ratio = (f32)frame_buffer.height/(f32)frame_buffer.width;
        f32 z_near = 0.1f;
        f32 z_far = 1000.0f;
        Matrix4x4 proj = matrix4x4_perspective(fovr, aspect_ratio, z_near, z_far);
    
        Matrix4x4 rot_x = {0};
        rot_x.m[0][0] = 1.0f;
        rot_x.m[1][1] = cos_f32(radians_f32(tick));
        rot_x.m[1][2] = sin_f32(radians_f32(tick));
        rot_x.m[2][1] = -sin_f32(radians_f32(tick));
        rot_x.m[2][2] = cos_f32(radians_f32(tick));
        rot_x.m[3][3] = 1.0f;

        Matrix4x4 rot_y = {0};
        rot_y.m[0][0] =  cos_f32(radians_f32(tick));
        rot_y.m[0][2] = -sin_f32(radians_f32(tick));
        rot_y.m[1][1] = 1.0f;
        rot_y.m[2][0] = sin_f32(radians_f32(tick));
        rot_y.m[2][2] = cos_f32(radians_f32(tick));
        rot_x.m[3][3] = 1.0f;

        Matrix4x4 rot_z = {0};
        rot_z.m[0][0] = cos_f32(radians_f32(tick));
        rot_z.m[0][1] = sin_f32(radians_f32(tick));
        rot_z.m[1][0] = -sin_f32(radians_f32(tick));
        rot_z.m[1][1] = cos_f32(radians_f32(tick));
        rot_z.m[2][2] = 1.0f;
        rot_z.m[3][3] = 1.0f;

        image_clear(frame_buffer, 0);
        for (u32 vertex_index = 0;
             vertex_index < array_count(vertices);
             vertex_index += 3) {
            Vector3 v0 = vertices[vertex_index];
            Vector3 v1 = vertices[vertex_index + 1];
            Vector3 v2 = vertices[vertex_index + 2];

            v0 = matrix4x4_mul(rot_z, v0);
            v1 = matrix4x4_mul(rot_z, v1);
            v2 = matrix4x4_mul(rot_z, v2);

            // v0 = matrix4x4_mul(rot_y, v0);
            // v1 = matrix4x4_mul(rot_y, v1);
            // v2 = matrix4x4_mul(rot_y, v2);

            v0 = matrix4x4_mul(rot_x, v0);
            v1 = matrix4x4_mul(rot_x, v1);
            v2 = matrix4x4_mul(rot_x, v2);

            v0.z += 2.0f;
            v1.z += 2.0f;
            v2.z += 2.0f;

            Vector3 d01 = vector3_sub(v1, v0);
            Vector3 d02 = vector3_sub(v2, v0);
            Vector3 dc0 = vector3_sub(v0, cam_p);

            Vector3 normal = vector3_normalize(vector3_cross(d01, d02));
            f32 scalar = vector3_dot(normal, dc0);

            if (scalar < 0.0f) {
                v0 = matrix4x4_mul(proj, v0);
                v1 = matrix4x4_mul(proj, v1);
                v2 = matrix4x4_mul(proj, v2);

                v0.x = (v0.x + 1.0f)*(f32)frame_buffer.width*0.5f;
                v1.x = (v1.x + 1.0f)*(f32)frame_buffer.width*0.5f;
                v2.x = (v2.x + 1.0f)*(f32)frame_buffer.width*0.5f;

                v0.y = (-v0.y + 1.0f)*(f32)frame_buffer.height*0.5f;
                v1.y = (-v1.y + 1.0f)*(f32)frame_buffer.height*0.5f;
                v2.y = (-v2.y + 1.0f)*(f32)frame_buffer.height*0.5f;

                fill_triangle(frame_buffer.pixels, frame_buffer.width, frame_buffer.height,
                              v0.x, v0.y, v1.x, v1.y, v2.x, v2.y, 0xc1c1c1);

                draw_line(frame_buffer.pixels, frame_buffer.width, frame_buffer.height, 
                          v0.x, v0.y, v1.x, v1.y, 0x79241f);
                draw_line(frame_buffer.pixels, frame_buffer.width, frame_buffer.height, 
                          v1.x, v1.y, v2.x, v2.y, 0x79241f);
                draw_line(frame_buffer.pixels, frame_buffer.width, frame_buffer.height, 
                          v2.x, v2.y, v0.x, v0.y, 0x79241f);
            }
        }

        GC gc = XCreateGC(display, window, 0, 0);
        XPutImage(display, window, gc, image, 0, 0, 0, 0, window_width, window_height);
        XFreeGC(display, gc);
        tick++;
    }

    XUnmapWindow(display, window);
    XDestroyWindow(display, window);
    XCloseDisplay(display);
    return(0);
}
