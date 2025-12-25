#ifndef KRUEGER_OPENGL_C
#define KRUEGER_OPENGL_C

#if PLATFORM_WINDOWS
#include "krueger_opengl_win32.c"
#else
#error opengl layer not implemented for the current platform.
#endif

#endif // KRUEGER_OPENGL_C
