#ifndef KRUEGER_PLATFORM_CORE_WIN32_C
#define KRUEGER_PLATFORM_CORE_WIN32_C

internal void
platform_core_init(void) {
  LARGE_INTEGER large_integer;
  QueryPerformanceFrequency(&large_integer);
  win32_core_state.us_res = large_integer.QuadPart;
  timeBeginPeriod(1);
}

internal void
platform_core_shutdown(void) {
  timeEndPeriod(1);
}

internal String8
platform_get_exec_file_path(Arena *arena) {
  char path[MAX_PATH];
  GetModuleFileName(0, path, MAX_PATH);
  uxx len = cstr_len(path);
  u8 *str = arena_push_array(arena, u8, len);
  mem_copy(str, path, len);
  String8 result = make_str8(str, len);
  return(result);
}

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

internal Platform_Handle
platform_file_open(char *filepath, Platform_File_Flags flags) {
  Platform_Handle result = {0};
  DWORD desired_access = 0;
  DWORD share_mode = 0;
  DWORD creation_disposition = OPEN_EXISTING;
  if (flags & PLATFORM_FILE_READ)        desired_access |= GENERIC_READ;
  if (flags & PLATFORM_FILE_WRITE)       desired_access |= GENERIC_WRITE;
  if (flags & PLATFORM_FILE_EXECUTE)     desired_access |= GENERIC_EXECUTE;
  if (flags & PLATFORM_FILE_SHARE_READ)  share_mode |= FILE_SHARE_READ;
  if (flags & PLATFORM_FILE_SHARE_WRITE) share_mode |= FILE_SHARE_WRITE | FILE_SHARE_DELETE;
  if (flags & PLATFORM_FILE_WRITE)       creation_disposition = CREATE_ALWAYS;
  HANDLE handle = CreateFileA(filepath, desired_access, share_mode, 0, creation_disposition, FILE_ATTRIBUTE_NORMAL, 0);
  if (handle != INVALID_HANDLE_VALUE) {
    result.ptr[0] = (u64)handle; 
  }
  return(result);
}

internal void
platform_file_close(Platform_Handle file) {
  HANDLE handle = (HANDLE)file.ptr[0];
  CloseHandle(handle);
}

internal u64
platform_file_read(Platform_Handle file, void *buffer, u64 size) {
  u64 read_size = 0;
  HANDLE handle = (HANDLE)file.ptr[0];
  ReadFile(handle, buffer, (DWORD)size, (DWORD *)&read_size, 0);
  return(read_size);
}

internal u64
platform_file_write(Platform_Handle file, void *buffer, u64 size) {
  u64 write_size = 0;
  HANDLE handle = (HANDLE)file.ptr[0];
  WriteFile(handle, buffer, (DWORD)size, (DWORD *)&write_size, 0);
  return(write_size);
}

internal u64
platform_get_file_size(Platform_Handle file) {
  u64 result = 0;
  HANDLE handle = (HANDLE)file.ptr[0];
  GetFileSizeEx(handle, (LARGE_INTEGER *)&result);
  return(result);
}

internal b32
platform_copy_file_path(char *dst, char *src) {
  b32 result = CopyFileA(src, dst, 0);
  return(result);
}

internal Platform_Handle
platform_library_open(char *filepath) {
  Platform_Handle result = {0};
  result.ptr[0] = (u64)LoadLibraryA(filepath);
  return(result);
}

internal void *
platform_library_load_proc(Platform_Handle lib, char *name) {
  HMODULE module = (HMODULE)lib.ptr[0];
  void *result = (void *)GetProcAddress(module, name);
  return(result);
}

internal void
platform_library_close(Platform_Handle lib) {
  HMODULE module = (HMODULE)lib.ptr[0];
  FreeLibrary(module);
}

internal u64
platform_get_time_us(void) {
  LARGE_INTEGER large_integer;
  QueryPerformanceCounter(&large_integer);
  u64 result = large_integer.QuadPart*million(1)/win32_core_state.us_res;
  return(result);
}

internal void
platform_sleep_ms(u32 ms) {
  Sleep(ms);
}

#endif // KRUEGER_PLATFORM_CORE_WIN32_C
