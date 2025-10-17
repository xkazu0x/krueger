#ifndef KRUEGER_BASE_ARENA_H
#define KRUEGER_BASE_ARENA_H

//////////////
// NOTE: Arena

typedef struct {
  uxx reserve_size;
  uxx commit_size;
  u8 *memory;
} Arena;

typedef struct {
  Arena *arena;
  uxx commit_size;
} Temp;

#define push_array(a, T, c) (T *)arena_push((a), sizeof(T)*(c))

internal Arena arena_alloc(uxx reserve_size);
internal void arena_release(Arena *arena);
internal void *arena_push(Arena *arena, uxx commit_size);

internal Temp temp_begin(Arena *arena);
internal void temp_end(Temp temp);

#endif // KRUEGER_BASE_ARENA_H
