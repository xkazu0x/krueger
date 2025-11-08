#ifndef KRUEGER_BASE_ARENA_H
#define KRUEGER_BASE_ARENA_H

typedef struct {
  uxx res_size;
  uxx cmt_size;
  u8 *base;
} Arena;

typedef struct {
  Arena *arena;
  uxx cmt_size;
} Temp;

internal Arena make_arena(u8 *base, uxx res_size);
internal Arena arena_alloc(uxx res_size);
internal void arena_release(Arena *arena);
internal void *arena_push(Arena *arena, uxx cmt_size);
internal void arena_clear(Arena *arena);

#define push_array(a, T, c) (T *)arena_push((a), sizeof(T)*(c))
#define push_struct(a, T) (T *)push_array((a), T, 1)

internal Temp temp_begin(Arena *arena);
internal void temp_end(Temp temp);

#endif // KRUEGER_BASE_ARENA_H
