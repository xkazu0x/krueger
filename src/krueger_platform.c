#ifndef KRUEGER_PLATFORM_C
#define KRUEGER_PLATFORM_C

#include "krueger_platform_core.c"
#if PLATFORM_WINDOWS
#include "krueger_platform_core_win32.c"
#elif PLATFORM_LINUX
#include "krueger_platform_core_linux.c"
#else
#error platform core not implemented for the current platform.
#endif

#if PLATFORM_FEATURE_GRAPHICS
#if PLATFORM_WINDOWS
#include "krueger_platform_graphics_win32.c"
// #elif PLATFORM_LINUX
// #include "krueger_platform_graphics_linux.c"
#else
#error platform graphics not implemented for the current platform.
#endif
#endif

#endif // KRUEGER_PLATFORM_C
