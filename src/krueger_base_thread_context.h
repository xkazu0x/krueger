#ifndef KRUEGER_BASE_THREAD_CONTEXT_H
#define KRUEGER_BASE_THREAD_CONTEXT_H

typedef struct {
  Arena *arenas[2];
} Thread_Context;

internal Thread_Context *thread_context_alloc(void);
internal void thread_context_release(Thread_Context *context);
internal void thread_context_select(Thread_Context *context);
internal Thread_Context *thread_context_selected(void);

internal Arena *thread_context_get_scratch(Arena **conflicts, u32 count);
#define scratch_begin(conflicts, count) temp_begin(thread_context_get_scratch((conflicts), (count)))
#define scratch_end(scratch) temp_end(scratch)

#endif // KRUEGER_BASE_THREAD_CONTEXT_H
