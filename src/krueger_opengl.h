#ifndef KRUEGER_OPENGL_H
#define KRUEGER_OPENGL_H

#if !defined(OGL_MAJOR_VER)
#error opengl major version must be defined.
#endif
#if !defined(OGL_MINOR_VER)
#error opengl minor version must be defined.
#endif

internal void *ogl_load_proc(char *name);
internal void ogl_init(void);
internal void ogl_window_equip(Platform_Handle handle);
internal void ogl_window_select(Platform_Handle handle);
internal void ogl_window_swap(Platform_Handle handle);

#if PLATFORM_WINDOWS
#include "krueger_opengl_win32.h"
#elif PLATFORM_LINUX
#include "krueger_opengl_linux.h"
#else
#error opengl layer not implemented for the current platform.
#endif

#endif // KRUEGER_OPENGL_H
