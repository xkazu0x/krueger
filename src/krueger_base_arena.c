#ifndef KRUEGER_BASE_ARENA_C
#define KRUEGER_BASE_ARENA_C

internal Arena
make_arena(u8 *base, uxx res_size) {
  Arena result = {
    .res_size = res_size,
    .cmt_size = 0,
    .base = base,
  };
  return(result);
}

internal Arena
arena_alloc(uxx res_size) {
  u8 *base = platform_reserve(res_size);
  platform_commit(base, res_size);
  Arena result = make_arena(base, res_size);
  return(result);
}

internal void
arena_release(Arena *arena) {
  platform_release(arena->base, arena->res_size);
  arena->res_size = 0;
  arena->cmt_size = 0;
  arena->base = 0;
}

internal void *
arena_push(Arena *arena, uxx cmt_size) {
  assert((arena->cmt_size + cmt_size) <= arena->res_size);
  void *result = (void *)(arena->base + arena->cmt_size);
  arena->cmt_size += cmt_size;
  return(result);
}

internal void
arena_clear(Arena *arena) {
  arena->cmt_size = 0;
}

internal Temp
temp_begin(Arena *arena) {
  Temp result = {
    .arena = arena,
    .cmt_size = arena->cmt_size,
  };
  return(result);
}

internal void
temp_end(Temp temp) {
  Arena *arena = temp.arena;
  arena->cmt_size = temp.cmt_size;
}

#endif // KRUEGER_BASE_ARENA_C
