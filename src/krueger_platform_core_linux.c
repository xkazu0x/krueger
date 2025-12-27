#ifndef KRUEGER_PLATFORM_CORE_LINUX_C
#define KRUEGER_PLATFORM_CORE_LINUX_C

///////////////////////////
// NOTE: Platform Functions

internal void
platform_core_init(void) {
}

internal void
platform_core_shutdown(void) {
}

internal void
platform_abort(s32 exit_code) {
  exit(exit_code);
}

internal Date_Time
platform_get_date_time(void) {
  Date_Time result = {0};
  return(result);
}

internal String8
platform_get_exec_file_path(Arena *arena) {
  Temp scratch = scratch_begin(&arena, 1);
  u8 *tmp = push_array(scratch.arena, u8, PATH_MAX);
  ssize_t len = readlink("/proc/self/exe", (char *)tmp, PATH_MAX);
  u8 *str = push_array(arena, u8, len + 1);
  mem_copy(str, tmp, len);
  str[len] = 0;
  String8 result = str8(str, len);
  scratch_end(scratch);
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
platform_file_open(String8 path, Platform_File_Flags flags) {
  Platform_Handle result = {0};
  Temp scratch = scratch_begin(0, 0);
  path = str8_copy(scratch.arena, path);
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
  int fd = open((char *)path.str, linux_flags, S_IRUSR | S_IWUSR);
  if (fd != -1) {
    result.ptr[0] = fd;
  }
  scratch_end(scratch);
  return(result);
}

internal void
platform_file_close(Platform_Handle file) {
  int fd = (int)file.ptr[0];
  close(fd);
}

internal u32
platform_file_read(Platform_Handle file, void *buffer, u64 size) {
  int fd = (int)file.ptr[0];
  u64 read_size = read(fd, buffer, size);
  return(read_size);
}

internal u32
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
platform_copy_file_path(String8 dst, String8 src) {
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
platform_library_open(String8 path) {
  Platform_Handle result = {0};
  Temp scratch = scratch_begin(0, 0);
  path = str8_copy(scratch.arena, path);
  result.ptr[0] = (uxx)dlopen((char *)path.str, RTLD_LAZY | RTLD_LOCAL);
  scratch_end(scratch);
  return(result);
}

internal void *
platform_library_load_proc(Platform_Handle lib, String8 name) {
  Temp scratch = scratch_begin(0, 0);
  name = str8_copy(scratch.arena, name);
  void *so = (void *)lib.ptr[0];
  void *result = (void *)dlsym(so, (char *)name.str);
  scratch_end(scratch);
  return(result);
}

internal void
platform_library_close(Platform_Handle lib) {
  void *so = (void *)lib.ptr[0];
  dlclose(so);
}

internal u64
platform_get_time_us(void) {
  struct timespec clock;
  clock_gettime(CLOCK_MONOTONIC, &clock);
  u64 result = clock.tv_sec*million(1) + clock.tv_nsec/thousand(1); 
  return(result);
}

internal void
platform_sleep_ms(u32 ms) {
  usleep(ms*thousand(1));
}

/////////////////////////////
// NOTE: Platform Entry Point

#if BUILD_ENTRY_POINT
int
main(int argc, char **argv) {
  base_entry_point(argc, argv);
  return(0);
}
#endif

#endif // KRUEGER_PLATFORM_CORE_LINUX_C
