#ifndef KRUEGER_BASE_ENTRY_POINT_C
#define KRUEGER_BASE_ENTRY_POINT_C

internal void
base_entry_point(int argc, char **argv) {
  platform_core_init();

#if defined(KRUEGER_PLATFORM_GRAPHICS_H)
  platform_graphics_init();
#endif
#if defined(KRUEGER_OPENGL_H)
  gl_init();
#endif

  Thread_Context *tctx = thread_context_alloc();
  thread_context_select(tctx);

  entry_point(argc, argv);

  thread_context_release(tctx);
  platform_core_shutdown();
}

#endif // KRUEGER_BASE_ENTRY_POINT_C
