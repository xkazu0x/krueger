#ifndef KRUEGER_BASE_H
#define KRUEGER_BASE_H

//////////////////////////////////
// NOTE: Standard Library Includes

#include <stdlib.h> // TODO: remove, only stretchy buffer needs it
#undef min
#undef max

#include <stdint.h>
#include <stddef.h>
#include <float.h>
#include <math.h>
#include <string.h>

//////////////////////
// NOTE: Base Includes

#include "krueger_base_context.h"
#include "krueger_base_core.h"
#include "krueger_base_math.h"
#include "krueger_base_arena.h"
#include "krueger_base_stretchy_buffer.h"
#include "krueger_base_string.h"

#endif // KRUEGER_BASE_H
