#ifndef KRUEGER_PLATFORM_WIN32_C
#define KRUEGER_PLATFORM_WIN32_C

#define NO_MIN_MAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

internal void *
platform_reserve(uxx size) {
  void *result = VirtualAlloc(0, size, MEM_RESERVE, PAGE_READWRITE);
  return(result);
}

internal b32
platform_commit(void *ptr, uxx size) {
  b32 result = (VirtualAlloc(ptr, size, MEM_COMMIT, PAGE_READWRITE) != 0);
  return(result);
}

internal void
platform_release(void *ptr) {
  if (ptr) VirtualFree(ptr, 0, MEM_RELEASE);
}

#endif // KRUEGER_PLATFORM_WIN32_C
