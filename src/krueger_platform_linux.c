#ifndef KRUEGER_PLATFORM_LINUX_C
#define KRUEGER_PLATFORM_LINUX_C

#include <sys/mman.h>
#include <stdio.h> // TODO: remove this

internal void *
platform_reserve(uxx size) {
  void *result = mmap(0, size, PROT_NONE , MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (result == MAP_FAILED) result = 0;
  return(result);
}

internal b32
platform_commit(void *ptr, uxx size) {
  mprotect(ptr, size, PROT_READ | PROT_WRITE);
  return(true);
}

internal void
platform_decommit(void *ptr, uxx size) {
  madvise(ptr, size, MADV_DONTNEED);
  mprotect(ptr, size, PROT_NONE);
}

internal void
platform_release(void *ptr, uxx size) {
  munmap(ptr, size);
}

#endif // KRUEGER_PLATFORM_LINUX_C
