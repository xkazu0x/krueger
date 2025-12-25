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
#ifndef UNICODE
#define UNICODE
#endif
#include <windows.h>
#include <timeapi.h>
#pragma comment(lib, "winmm")

////////////////////////
// NOTE: Windows Globals

global u64 _win32_us_res;

#endif // KRUEGER_PLATFORM_CORE_WIN32_H
