#ifndef KRUEGER_BASE_ARENA_C
#define KRUEGER_BASE_ARENA_C

//////////////
// NOTE: Arena

internal Arena
arena_alloc(uxx reserve_size) {
  Arena result = {0};
  result.reserve_size = reserve_size;
  result.commit_size = 0;
  result.memory = platform_reserve(reserve_size);
  platform_commit(result.memory, reserve_size);
  return(result);
}

internal void
arena_release(Arena *arena) {
  platform_release(arena->memory, arena->reserve_size);
}

internal void *
arena_push(Arena *arena, uxx commit_size) {
  assert((arena->commit_size + commit_size) <= arena->reserve_size);
  void *result = (void *)(arena->memory + arena->commit_size);
  arena->commit_size += commit_size;
  return(result);
}

internal Temp
temp_begin(Arena *arena) {
  Temp result = {0};
  result.arena = arena;
  result.commit_size = arena->commit_size;
  return(result);
}

internal void
temp_end(Temp temp) {
  Arena *arena = temp.arena;
  arena->commit_size = temp.commit_size;
}

#endif // KRUEGER_BASE_ARENA_C
