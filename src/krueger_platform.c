#ifndef KRUEGER_PLATFORM_C
#define KRUEGER_PLATFORM_C

#include "krueger_platform_core.c"

#if PLATFORM_WINDOWS
#include "krueger_platform_core_win32.c"
#elif PLATFORM_LINUX
#include "krueger_platform_core_linux.c"
#else
#error platform core not supported for the current platform
#endif

#if PLATFORM_GFX
#if PLATFORM_WINDOWS
#include "krueger_platform_gfx_win32.c"
#elif PLATFORM_LINUX
#include "krueger_platform_gfx_linux.c"
#else
#error platform gfx not implemented for the current platform
#endif
#endif

#endif // KRUEGER_PLATFORM_C
