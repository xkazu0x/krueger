#ifndef KRUEGER_BASE_H
#define KRUEGER_BASE_H

#if !defined(KRUEGER_BASE_STRETCHY_BUFFER)
#define KRUEGER_BASE_STRETCHY_BUFFER 0
#endif

#include "krueger_base_context.h"
#include "krueger_base_core.h"
#include "krueger_base_array.h"

#include "krueger_base_math.h"
#include "krueger_base_arena.h"
#include "krueger_base_string.h"
#include "krueger_base_log.h"

#if KRUEGER_BASE_STRETCHY_BUFFER
#include "krueger_base_stretchy_buffer.h"
#endif

#endif // KRUEGER_BASE_H
