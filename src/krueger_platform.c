#ifndef KRUEGER_PLATFORM_C
#define KRUEGER_PLATFORM_C

internal b32
platform_handle_match(Platform_Handle a, Platform_Handle b) {
  b32 result = (a.ptr[0] == b.ptr[0]);
  return(result);
}

#if PLATFORM_WINDOWS
#include "krueger_platform_win32.c"
#elif PLATFORM_LINUX
#include "krueger_platform_linux.c"
#else
#error current platform is not supported
#endif

#endif // KRUEGER_PLATFORM_C
