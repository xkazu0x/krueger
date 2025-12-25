#ifndef KRUEGER_BASE_ARENA_C
#define KRUEGER_BASE_ARENA_C

////////////////////////
// NOTE: Arena Functions

internal Arena *
_arena_alloc(Arena_Desc *desc) {
  uxx res_size = clamp_bot(ARENA_HEADER_SIZE, desc->res_size);
  void *base = desc->base;
  if (!base) {
    base = platform_reserve(res_size);
    platform_commit(base, res_size);
  }
  Arena *result = (Arena *)base;
  result->res_size = res_size;
  result->pos = ARENA_HEADER_SIZE;
  return(result);
}

internal void
arena_release(Arena *arena) {
  platform_release(arena, arena->res_size);
}

internal void *
arena_push(Arena *arena, uxx size) {
  void *result = 0;
  if ((arena->pos + size) <= arena->res_size) {
    result = (u8 *)arena + arena->pos;
    arena->pos += size;
    mem_zero(result, size);
  }
  assert(result != 0);
  return(result);
}

internal void
arena_pop_to(Arena *arena, uxx pos) {
  uxx pos_new = clamp_bot(ARENA_HEADER_SIZE, pos);
  assert(pos_new <= arena->pos);
  arena->pos = pos_new;
}

internal void
arena_pop(Arena *arena, uxx size) {
  uxx pos_old = arena->pos;
  uxx pos_new = pos_old;
  if (size < pos_old) {
    pos_new = pos_old - size;
  }
  arena_pop_to(arena, pos_new);
}

internal void
arena_clear(Arena *arena) {
  arena_pop_to(arena, 0);
}

// NOTE: Temporary Arena
internal Temp
temp_begin(Arena *arena) {
  Temp result = {
    .arena = arena,
    .pos = arena->pos,
  };
  return(result);
}

internal void
temp_end(Temp temp) {
  arena_pop_to(temp.arena, temp.pos);
}

#endif // KRUEGER_BASE_ARENA_C
