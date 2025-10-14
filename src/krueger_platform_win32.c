#ifndef KRUEGER_PLATFORM_WIN32_C
#define KRUEGER_PLATFORM_WIN32_C

#ifndef NO_MIN_MAX
#define NO_MIN_MAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <stdio.h> // TODO: remove this

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
platform_decommit(void *ptr, uxx size) {
  VirtualFree(ptr, size, MEM_DECOMMIT);
}

internal void
platform_release(void *ptr, uxx size) {
  VirtualFree(ptr, 0, MEM_RELEASE);
}

internal void *
platform_read_file(char *filename) {
  void *result = 0;
  FILE *file = fopen(filename, "rb");
  if (file) {
    fseek(file, 0, SEEK_END);
    uxx size = ftell(file);
    fseek(file, 0, SEEK_SET);
    result = platform_reserve(size);
    platform_commit(result, size);
    fread(result, size, 1, file);
    fclose(file);
  }
  return(result);
}

#endif // KRUEGER_PLATFORM_WIN32_C
