#ifndef KRUEGER_PLATFORM_H
#define KRUEGER_PLATFORM_H

#if !defined(PLATFORM_GFX)
#define PLATFORM_GFX 0
#endif

#include "krueger_platform_meta.h"

#include "krueger_platform_core.h"
#ifdef PLATFORM_GFX
#include "krueger_platform_gfx.h"
#endif

#if PLATFORM_WINDOWS
#include "krueger_platform_core_win32.h"
#elif PLATFORM_LINUX
#include "krueger_platform_core_linux.h"
#else
#error platform core not supported for the current platform
#endif

#if PLATFORM_GFX
#if PLATFORM_WINDOWS
#include "krueger_platform_gfx_win32.h"
#elif PLATFORM_LINUX
#include "krueger_platform_gfx_linux.h"
#else
#error platform gfx not implemented for the current platform
#endif
#endif

#endif // KRUEGER_PLATFORM_H
