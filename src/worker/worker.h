#pragma once

#include "mish_common.h"

/* This is fine for now. */
typedef output_slot decode_result;

typedef void (*try_decode_t)(decode_result *result, uint8_t *raw_insn, uint8_t length,
                             decoder_mode mode);
