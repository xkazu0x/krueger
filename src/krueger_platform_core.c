#ifndef KRUEGER_PLATFORM_CORE_C
#define KRUEGER_PLATFORM_CORE_C

internal b32
platform_handle_match(Platform_Handle a, Platform_Handle b) {
  b32 result = (a.ptr[0] == b.ptr[0]);
  return(result);
}

#endif // KRUEGER_PLATFORM_CORE_C
