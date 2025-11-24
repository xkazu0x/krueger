#ifndef KRUEGER_PLATFORM_H
#define KRUEGER_PLATFORM_H

#if !defined(KRUEGER_PLATFORM_GRAPHICS)
#define KRUEGER_PLATFORM_GRAPHICS 0
#endif

#include "krueger_platform_core.h"
#include "krueger_platform_meta.h"
#if KRUEGER_PLATFORM_GRAPHICS
#include "krueger_platform_graphics.h"
#endif

#if PLATFORM_WINDOWS
#include "krueger_platform_core_win32.h"
#elif PLATFORM_LINUX
#include "krueger_platform_core_linux.h"
#else
#error platform core not implemented for the current platform.
#endif

#if KRUEGER_PLATFORM_GRAPHICS
#if PLATFORM_WINDOWS
#include "krueger_platform_graphics_win32.h"
// #elif PLATFORM_LINUX
// #include "krueger_platform_graphics_linux.h"
#else
#error platform graphics not implemented for the current platform.
#endif
#endif

#endif // KRUEGER_PLATFORM_H
