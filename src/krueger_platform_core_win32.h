#ifndef KRUEGER_PLATFORM_CORE_WIN32_H
#define KRUEGER_PLATFORM_CORE_WIN32_H

#define NO_MIN_MAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

typedef struct {
  u64 us_res;
} Win32_Core_State;

////////////////////////////////////
// NOTE: Win32 Core Global Variables

global Win32_Core_State win32_core_state;

#endif // KRUEGER_PLATFORM_CORE_WIN32_H
