#ifndef KRUEGER_BASE_ARENA_H
#define KRUEGER_BASE_ARENA_H

typedef struct {
  uxx res_size;
  uxx cmt_size;
  void *base;
} Arena_Params;

typedef struct {
  uxx res_size;
  uxx cmt_size;
  u8 *base;
} Arena;

typedef struct {
  Arena *arena;
  uxx cmt_size;
} Temp;

global uxx arena_default_res_size = MB(64);
global uxx arena_default_cmt_size = sizeof(Arena);

#define arena_alloc(...) _arena_alloc(&(Arena_Params){ \
  .res_size = arena_default_res_size, \
  .cmt_size = arena_default_cmt_size, \
  __VA_ARGS__ \
})
internal Arena *_arena_alloc(Arena_Params *params);
internal void arena_release(Arena *arena);

#define push_array(a, T, c) (T *)arena_push((a), sizeof(T)*(c))
#define push_struct(a, T) (T *)push_array((a), T, 1)
internal void *arena_push(Arena *arena, uxx cmt_size);
internal void arena_clear(Arena *arena);

internal Temp temp_begin(Arena *arena);
internal void temp_end(Temp temp);

#endif // KRUEGER_BASE_ARENA_H
