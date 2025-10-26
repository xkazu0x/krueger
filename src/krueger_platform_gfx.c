#ifndef KRUEGER_PLATFORM_GFX_C
#define KRUEGER_PLATFORM_GFX_C

internal void
platform_gfx_init(void) {
  platform_events.cap = KEY_MAX;
  platform_events.len = 0;
  uxx event_array_size = platform_events.cap*sizeof(Platform_Event);
  platform_events.items = platform_reserve(event_array_size);
  platform_commit(platform_events.items, event_array_size);
}

#endif // KRUEGER_PLATFORM_GFX_C
