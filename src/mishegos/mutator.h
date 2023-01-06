#pragma once

#include "mish_common.h"

#include <stdbool.h>

/* Generate a single fuzzing candidate and populate the given input slot with it.
 * Returns false if the configured mutation mode has been exhausted.
 */
typedef bool (*mutator_t)(input_slot *);

mutator_t mutator_create(const char *name);
