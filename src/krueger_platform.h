#ifndef KRUEGER_PLATFORM_H
#define KRUEGER_PLATFORM_H

#if !defined(PLATFORM_FEATURE_GRAPHICS)
#define PLATFORM_FEATURE_GRAPHICS 0
#endif

#include "krueger_platform_core.h"
#if PLATFORM_FEATURE_GRAPHICS
#include "krueger_keycode.h"
#include "krueger_platform_graphics.h"
#endif

#if PLATFORM_WINDOWS
#include "krueger_platform_core_win32.h"
#elif PLATFORM_LINUX
#include "krueger_platform_core_linux.h"
#else
#error platform core not implemented for the current platform.
#endif

#if PLATFORM_FEATURE_GRAPHICS
#if PLATFORM_WINDOWS
#include "krueger_platform_graphics_win32.h"
// #elif PLATFORM_LINUX
// #include "krueger_platform_graphics_linux.h"
#else
#error platform graphics not implemented for the current platform.
#endif
#endif

#endif // KRUEGER_PLATFORM_H
