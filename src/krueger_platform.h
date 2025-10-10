#ifndef KRUEGER_PLATFORM_H
#define KRUEGER_PLATFORM_H

internal void *platform_reserve(uxx size);
internal b32 platform_commit(void *ptr, uxx size);
internal void platform_release(void *ptr);

#endif // KRUEGER_PLATFORM_H
