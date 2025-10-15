#ifndef KRUEGER_PLATFORM_WIN32_C
#define KRUEGER_PLATFORM_WIN32_C

#ifndef NO_MIN_MAX
#define NO_MIN_MAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
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
platform_decommit(void *ptr, uxx size) {
  VirtualFree(ptr, size, MEM_DECOMMIT);
}

internal void
platform_release(void *ptr, uxx size) {
  VirtualFree(ptr, 0, MEM_RELEASE);
}

internal Platform_Handle
platform_file_open(char *filepath, Platform_Access_Flags flags) {
  Platform_Handle result = {0};
  DWORD desired_access = 0;
  DWORD share_mode = 0;
  DWORD creation_disposition = OPEN_EXISTING;
  if (flags & PLATFORM_ACCESS_READ)        desired_access |= GENERIC_READ;
  if (flags & PLATFORM_ACCESS_WRITE)       desired_access |= GENERIC_WRITE;
  if (flags & PLATFORM_ACCESS_EXECUTE)     desired_access |= GENERIC_EXECUTE;
  if (flags & PLATFORM_ACCESS_SHARE_READ)  share_mode |= FILE_SHARE_READ;
  if (flags & PLATFORM_ACCESS_SHARE_WRITE) share_mode |= FILE_SHARE_WRITE | FILE_SHARE_DELETE;
  if (flags & PLATFORM_ACCESS_WRITE)       creation_disposition = CREATE_ALWAYS;
  HANDLE handle = CreateFileA(filepath, desired_access, share_mode, 0, creation_disposition, FILE_ATTRIBUTE_NORMAL, 0);
  if (handle != INVALID_HANDLE_VALUE) {
    result.ptr[0] = (uxx)handle; 
  }
  return(result);
}

internal void
platform_file_close(Platform_Handle file) {
  HANDLE handle = (HANDLE)file.ptr[0];
  CloseHandle(handle);
}

internal u64
platform_file_get_size(Platform_Handle file) {
  HANDLE handle = (HANDLE)file.ptr[0];
  u64 size = 0;
  GetFileSizeEx(handle, (LARGE_INTEGER *)&size);
  return(size);
}

internal u64
platform_file_read(Platform_Handle file, void *buffer, u64 size) {
  HANDLE handle = (HANDLE)file.ptr[0];
  u64 read_size = 0;
  ReadFile(handle, buffer, (DWORD)size, (DWORD *)&read_size, 0);
  return(read_size);
}

internal u64
platform_file_write(Platform_Handle file, void *buffer, u64 size) {
  HANDLE handle = (HANDLE)file.ptr[0];
  u64 write_size = 0;
  WriteFile(handle, buffer, (DWORD)size, (DWORD *)&write_size, 0);
  return(write_size);
}

internal Platform_Handle
platform_library_open(char *filepath) {
  Platform_Handle result = {0};
  result.ptr[0] = (uxx)LoadLibraryA(filepath);
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

#endif // KRUEGER_PLATFORM_WIN32_C
