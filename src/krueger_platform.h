#ifndef KRUEGER_PLATFORM_H
#define KRUEGER_PLATFORM_H

typedef u32 Platform_Access_Flags;
enum {
  PLATFORM_ACCESS_READ        = (1<<0),
  PLATFORM_ACCESS_WRITE       = (1<<1),
  PLATFORM_ACCESS_EXECUTE     = (1<<2),
  PLATFORM_ACCESS_SHARE_READ  = (1<<3),
  PLATFORM_ACCESS_SHARE_WRITE = (1<<4),
};

typedef struct {
  uxx ptr[1];
} Platform_Handle;

internal void *platform_reserve(uxx size);
internal b32 platform_commit(void *ptr, uxx size);
internal void platform_decommit(void *ptr, uxx size);
internal void platform_release(void *ptr, uxx size);

internal Platform_Handle platform_file_open(char *filepath, Platform_Access_Flags flags);
internal void platform_file_close(Platform_Handle file);
internal u64 platform_file_get_size(Platform_Handle file);
internal u64 platform_file_read(Platform_Handle file, void *buffer, u64 size);
internal u64 platform_file_write(Platform_Handle file, void *buffer, u64 size);

internal Platform_Handle platform_library_open(char *filepath);
internal void *platform_library_load_proc(Platform_Handle lib, char *name);
internal void platform_library_close(Platform_Handle lib);

#endif // KRUEGER_PLATFORM_H
