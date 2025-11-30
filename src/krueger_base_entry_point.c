#ifndef KRUEGER_BASE_ENTRY_POINT_C
#define KRUEGER_BASE_ENTRY_POINT_C

internal void
base_entry_point(int argc, char **argv) {
  platform_core_init();
#if defined(KRUEGER_PLATFORM_GRAPHICS_H)
  platform_graphics_init();
#endif
  Thread_Context *thread_context = thread_context_alloc();
  thread_context_select(thread_context);

  entry_point(argc, argv);

  thread_context_release(thread_context);
  platform_core_shutdown();
}

#endif // KRUEGER_BASE_ENTRY_POINT_C
