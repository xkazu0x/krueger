#ifndef KRUEGER_BASE_C
#define KRUEGER_BASE_C

#include "krueger_base_core.c"
#include "krueger_base_arena.c"
#include "krueger_base_string.c"
#include "krueger_base_math.c"
#include "krueger_base_thread_context.c"
#include "krueger_base_log.c"

#if BUILD_ENTRY_POINT
#include "krueger_base_entry_point.c"
#endif

#endif // KRUEGER_BASE_C
