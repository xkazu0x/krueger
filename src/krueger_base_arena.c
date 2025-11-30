#ifndef KRUEGER_BASE_ARENA_C
#define KRUEGER_BASE_ARENA_C

internal Arena *
_arena_alloc(Arena_Params *params) {
  assert(params->res_size >= arena_default_cmt_size);
  assert(params->cmt_size >= arena_default_cmt_size);

  uxx res_size = params->res_size;
  uxx cmt_size = params->cmt_size;

  void *base = params->base;
  if (!base) {
    base = platform_reserve(res_size);
    platform_commit(base, res_size);
  }

  Arena *result = (Arena *)base;
  result->res_size = res_size;
  result->cmt_size = cmt_size;
  result->base = base;

  return(result);
}

internal void *
arena_push(Arena *arena, uxx cmt_size) {
  void *result = 0;
  if ((arena->cmt_size + cmt_size) <= arena->res_size) {
    result = arena->base + arena->cmt_size;
    arena->cmt_size += cmt_size;
    mem_zero(result, cmt_size);
  }
  assert(result != 0);
  return(result);
}

internal void
arena_release(Arena *arena) {
  platform_release(arena->base, arena->res_size);
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
