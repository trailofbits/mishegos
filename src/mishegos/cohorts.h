#pragma once

#include "mish_common.h"

void cohorts_init();
void cohorts_cleanup();
bool add_to_cohort(output_slot *slot);
void dump_cohorts();
