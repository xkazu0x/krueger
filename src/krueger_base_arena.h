#ifndef KRUEGER_BASE_ARENA_H
#define KRUEGER_BASE_ARENA_H

////////////////////
// NOTE: Arena Types

typedef struct {
  uxx res_size;
  void *base;
} Arena_Desc;

typedef struct {
  uxx res_size;
  uxx pos;
} Arena;

typedef struct {
  Arena *arena;
  uxx pos;
} Temp;

#define ARENA_HEADER_SIZE (sizeof(Arena))

global const uxx arena_default_res_size = MB(64);

////////////////////////
// NOTE: Arena Functions

#define arena_alloc(...) _arena_alloc(&(Arena_Desc){ \
  .res_size = arena_default_res_size, \
  __VA_ARGS__ \
})
internal Arena *_arena_alloc(Arena_Desc *desc);
internal void arena_release(Arena *arena);

internal void *arena_push(Arena *arena, uxx cmt_size);
internal void arena_pop_to(Arena *arena, uxx pos);
internal void arena_pop(Arena *arena, uxx size);
internal void arena_clear(Arena *arena);

// NOTE: Temporary Arena
internal Temp temp_begin(Arena *arena);
internal void temp_end(Temp temp);

// NOTE: Push Helper Macros
#define push_struct(a, T)   (T *)arena_push((a), sizeof(T))
#define push_array(a, T, c) (T *)arena_push((a), sizeof(T)*(c))

#endif // KRUEGER_BASE_ARENA_H
