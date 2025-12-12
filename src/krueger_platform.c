#ifndef KRUEGER_PLATFORM_C
#define KRUEGER_PLATFORM_C

#include "krueger_platform_core.c"
#if PLATFORM_WINDOWS
#include "krueger_platform_core_win32.c"
#else
#error platform core not implemented for the current platform.
#endif

#if PLATFORM_FEATURE_GRAPHICS
#if PLATFORM_WINDOWS
#include "krueger_platform_graphics_win32.c"
#else
#error platform graphics not implemented for the current platform.
#endif
#endif

#if PLATFORM_FEATURE_AUDIO
#include "krueger_platform_audio.c"
#if PLATFORM_WINDOWS
#include "krueger_platform_audio_win32.c"
#else
#error platform audio not implemented for the current platform.
#endif
#endif

#endif // KRUEGER_PLATFORM_C
