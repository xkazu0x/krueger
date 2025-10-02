#ifndef PLATFORM_C
#define PLATFORM_C

#if PLATFORM_LINUX
#include "platform_linux.c"
#else
#error this platform is not supported
#endif

#endif // PLATFORM_C
