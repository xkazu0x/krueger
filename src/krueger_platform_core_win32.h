#ifndef KRUEGER_PLATFORM_CORE_WIN32_H
#define KRUEGER_PLATFORM_CORE_WIN32_H

/////////////////////////////
// NOTE: Includes / Libraries

#define NO_MIN_MAX
#include <windows.h>
#pragma comment(lib, "winmm")

//////////////
// NOTE: Types

typedef struct {
  u64 us_res;
} Win32_Core_State;

////////////////////////
// NOTE: Windows Globals

global Win32_Core_State win32_core_state;

#endif // KRUEGER_PLATFORM_CORE_WIN32_H
