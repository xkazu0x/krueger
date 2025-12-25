#ifndef KRUEGER_BASE_THREAD_CONTEXT_C
#define KRUEGER_BASE_THREAD_CONTEXT_C

thread_static Thread_Context *tctx_thread_local;

internal Thread_Context *
thread_context_alloc(void) {
  Arena *arena = arena_alloc();
  Thread_Context *result = push_array(arena, Thread_Context, 1);
  result->arenas[0] = arena;
  result->arenas[1] = arena_alloc();
  return(result);
}

internal void
thread_context_release(Thread_Context *tctx) {
  arena_release(tctx->arenas[1]);
  arena_release(tctx->arenas[0]);
}

internal void
thread_context_select(Thread_Context *tctx) {
  tctx_thread_local = tctx;
}

internal Thread_Context *
thread_context_selected(void) {
  return(tctx_thread_local);
}

internal Arena *
thread_context_get_scratch(Arena **conflicts, u32 count) {
  Arena *result = 0;
  Thread_Context *tctx = thread_context_selected();
  Arena **arena_ptr = tctx->arenas;
  for (u32 i = 0; i < array_count(tctx->arenas); ++i, ++arena_ptr) {
    Arena **conflict_ptr = conflicts;
    b32 has_conflict = false;
    for (u32 j = 0; j < count; ++j, ++conflict_ptr) {
      if (*arena_ptr == *conflict_ptr) {
        has_conflict = true;
        break;
      }
    }
    if (!has_conflict) {
      result = *arena_ptr;
      break;
    }
  }
  return(result);
}

#endif // KRUEGER_BASE_THREAD_CONTEXT_C
