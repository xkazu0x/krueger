#ifndef KRUEGER_PLATFORM_CORE_C
#define KRUEGER_PLATFORM_CORE_C

internal b32
platform_handle_match(Platform_Handle a, Platform_Handle b) {
  b32 result = (a.ptr[0] == b.ptr[0]);
  return(result);
}

internal void *
platform_read_entire_file(Arena *arena, char *path) {
  void *result = 0;
  Platform_Handle file = platform_file_open(path, PLATFORM_FILE_READ | PLATFORM_FILE_SHARE_READ);
  if (!platform_handle_match(file, PLATFORM_HANDLE_NULL)) {
    u64 file_size = platform_get_file_size(file);
    result = arena_push(arena, file_size);
    platform_file_read(file, result, file_size);
    platform_file_close(file);
  }
  return(result);
}

#endif // KRUEGER_PLATFORM_CORE_C
