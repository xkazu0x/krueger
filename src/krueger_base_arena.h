#ifndef KRUEGER_BASE_ARENA_H
#define KRUEGER_BASE_ARENA_H

typedef struct {
  uxx res_size;
  uxx cmt_size;
  u8 *buf;
} Arena;

typedef struct {
  Arena *arena;
  uxx cmt_size;
} Temp;

#define arena_push_array(a, T, c) (T *)arena_push((a), sizeof(T)*(c))
internal Arena make_arena(u8 *buf, uxx res_size);
internal void *arena_push(Arena *arena, uxx cmt_size);
internal void arena_clear(Arena *arena);

internal Temp temp_begin(Arena *arena);
internal void temp_end(Temp temp);

#endif // KRUEGER_BASE_ARENA_H
