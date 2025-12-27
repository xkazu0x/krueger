#ifndef KRUEGER_PLATFORM_H
#define KRUEGER_PLATFORM_H

#if !defined(PLATFORM_FEATURE_GRAPHICS)
#define PLATFORM_FEATURE_GRAPHICS 0
#endif
#if !defined(PLATFORM_FEATURE_AUDIO)
#define PLATFORM_FEATURE_AUDIO 0
#endif

#include "krueger_platform_core.h"
#if PLATFORM_WINDOWS
#include "krueger_platform_core_win32.h"
#elif PLATFORM_LINUX
#include "krueger_platform_core_linux.h"
#else
#error platform core not implemented for the current platform.
#endif

#if PLATFORM_FEATURE_GRAPHICS
#include "krueger_keycode.h"
#include "krueger_platform_graphics.h"
#if PLATFORM_WINDOWS
#include "krueger_platform_graphics_win32.h"
#elif PLATFORM_LINUX
#include "krueger_platform_graphics_linux.h"
#else
#error platform graphics not implemented for the current platform.
#endif
#endif

#if PLATFORM_FEATURE_AUDIO
#include "krueger_platform_audio.h"
#if PLATFORM_WINDOWS
#include "krueger_platform_audio_win32.h"
#elif PLATFORM_LINUX
// TODO: implement linux audio layer
#else
#error platform audio not implemented for the current platform.
#endif
#endif

#endif // KRUEGER_PLATFORM_H
