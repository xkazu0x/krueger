#ifndef KRUEGER_PLATFORM_C
#define KRUEGER_PLATFORM_C

#if PLATFORM_WINDOWS
#include "krueger_platform_win32.c"
#elif PLATFORM_LINUX
#include "krueger_platform_linux.c"
#else
#error current platform is not supported
#endif

#endif // KRUEGER_PLATFORM_C
