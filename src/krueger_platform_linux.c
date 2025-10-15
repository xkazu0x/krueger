#ifndef KRUEGER_PLATFORM_LINUX_C
#define KRUEGER_PLATFORM_LINUX_C

#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <dlfcn.h>

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

internal Platform_Handle
platform_file_open(char *filepath, Platform_File_Flags flags) {
  Platform_Handle result = {0};
  int linux_flags = 0;
  if ((flags & PLATFORM_FILE_READ) && (flags & PLATFORM_FILE_WRITE)) {
    linux_flags = O_RDWR;
  } else if(flags & PLATFORM_FILE_WRITE) {
    linux_flags = O_WRONLY;
  } else if(flags & PLATFORM_FILE_READ) {
    linux_flags = O_RDONLY;
  }
  if(flags & PLATFORM_FILE_WRITE) {
    linux_flags |= O_CREAT;
  }
  int fd = open(filepath, linux_flags, S_IRUSR | S_IWUSR);
  if (fd != -1) {
    result.ptr[0] = fd;
  }
  return(result);
}

internal void
platform_file_close(Platform_Handle file) {
  int fd = (int)file.ptr[0];
  close(fd);
}

internal u64
platform_file_get_size(Platform_Handle file) {
  u64 result = 0;
  int fd = (int)file.ptr[0];
  struct stat st;
  if (fstat(fd, &st) == 0) {
    result = st.st_size;
  }
  return(result);
}

internal u64
platform_file_read(Platform_Handle file, void *buffer, u64 size) {
  int fd = (int)file.ptr[0];
  u64 result = read(fd, buffer, size);
  return(result);
}

internal u64
platform_file_write(Platform_Handle file, void *buffer, u64 size) {
  int fd = (int)file.ptr[0];
  u64 result = write(fd, buffer, size);
  return(result);
}

internal Platform_Handle
platform_library_open(char *filepath) {
  Platform_Handle result = {0};
  result.ptr[0] = (u64)dlopen(filepath, RTLD_LAZY | RTLD_LOCAL);
  return(result);
}

internal void *
platform_library_load_proc(Platform_Handle lib, char *name) {
  void *so = (void *)lib.ptr[0];
  void *result = (void *)dlsym(so, name);
  return(result);
}

internal void
platform_library_close(Platform_Handle lib) {
  void *so = (void *)lib.ptr[0];
  dlclose(so);
}

#endif // KRUEGER_PLATFORM_LINUX_C
