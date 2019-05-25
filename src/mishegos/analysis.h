#pragma once

#include "mish_common.h"

void analysis_init();
void analysis_cleanup();
bool add_to_cohort(output_slot *slot);
void pump_cohorts();
