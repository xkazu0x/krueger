#ifndef KRUEGER_BASE_ENTRY_POINT_C
#define KRUEGER_BASE_ENTRY_POINT_C

internal void
base_entry_point(int argc, char **argv) {
  Thread_Context *tctx = thread_context_alloc();
  thread_context_select(tctx);
  platform_core_init();
#if defined(KRUEGER_PLATFORM_GRAPHICS_H)
  platform_graphics_init();
#endif
#if defined(KRUEGER_OPENGL_H)
  ogl_init();
#endif
  entry_point(argc, argv);
  platform_core_shutdown();
  thread_context_release(tctx);
}

#endif // KRUEGER_BASE_ENTRY_POINT_C
