#ifndef KRUEGER_PLATFORM_CORE_LINUX_C
#define KRUEGER_PLATFORM_CORE_LINUX_C

internal void
platform_init_core(void) {
}

internal String8
platform_get_exec_file_path(Arena *arena) {
  char *path = push_array(arena, char, PATH_MAX);
  ssize_t length = readlink("/proc/self/exe", path, PATH_MAX);
  if (length != -1) path[length] = 0;
  String8 result = str8_cstr(path);
  return(result);
}

internal u64
platform_get_time_us(void) {
  struct timespec clock;
  clock_gettime(CLOCK_MONOTONIC, &clock);
  u64 result = clock.tv_sec*million(1) + clock.tv_nsec/thousand(1); 
  return(result);
}

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
platform_file_read(Platform_Handle file, void *buffer, u64 size) {
  int fd = (int)file.ptr[0];
  u64 read_size = read(fd, buffer, size);
  return(read_size);
}

internal u64
platform_file_write(Platform_Handle file, void *buffer, u64 size) {
  int fd = (int)file.ptr[0];
  u64 write_size = write(fd, buffer, size);
  return(write_size);
}

internal u64
platform_get_file_size(Platform_Handle file) {
  u64 result = 0;
  int fd = (int)file.ptr[0];
  struct stat st;
  if (fstat(fd, &st) == 0) {
    result = st.st_size;
  }
  return(result);
}

internal b32
platform_copy_file_path(char *dst, char *src) {
  b32 result = false;
  Platform_Handle src_h = platform_file_open(src, PLATFORM_FILE_READ);
  Platform_Handle dst_h = platform_file_open(dst, PLATFORM_FILE_WRITE);
  if (!platform_handle_match(src_h, PLATFORM_HANDLE_NULL) &&
      !platform_handle_match(dst_h, PLATFORM_HANDLE_NULL)) {
    int src_fd = (int)src_h.ptr[0];
    int dst_fd = (int)dst_h.ptr[0];
    off_t sendfile_off = 0;
    u64 size = platform_get_file_size(src_h);
    u32 write_size = sendfile(dst_fd, src_fd, &sendfile_off, size);
    if (write_size == size) {
      result = true;
    }
    platform_file_close(src_h);
    platform_file_close(dst_h);
  }
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

#endif // KRUEGER_PLATFORM_CORE_LINUX_C
