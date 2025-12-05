#ifndef KRUEGER_PLATFORM_CORE_WIN32_H
#define KRUEGER_PLATFORM_CORE_WIN32_H

/////////////////////////////
// NOTE: Includes / Libraries

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NO_MIN_MAX
#define NO_MIN_MAX
#endif
#include <windows.h>
#include <timeapi.h>
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
