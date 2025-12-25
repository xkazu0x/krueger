#ifndef KRUEGER_OPENGL_H
#define KRUEGER_OPENGL_H

#if !defined(OPENGL_MAJOR_VERSION)
#error opengl major version must be defined.
#endif
#if !defined(OPENGL_MINOR_VERSION)
#error opengl minor version must be defined.
#endif

internal void gl_init(void);
internal void gl_window_equip(Platform_Handle handle);
internal void gl_window_select(Platform_Handle handle);
internal void gl_window_swap(Platform_Handle handle);

#if PLATFORM_WINDOWS
#include "krueger_opengl_win32.h"
#else
#error opengl layer not implemented for the current platform.
#endif

#endif // KRUEGER_OPENGL_H
