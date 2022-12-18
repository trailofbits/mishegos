#pragma once

#include "mish_core.h"

/* An x86 instruction's opcode is no longer than 5 bytes.
 * The opcode also includes VEX/XOP/EVEX prefixes.
 */
typedef struct __attribute__((packed)) {
  uint8_t len;
  uint8_t op[5];
} opcode;
static_assert(sizeof(opcode) == 6, "opcode should be 6 bytes");

/* An x86 instruction is no longer than 15 bytes,
 * but the longest (potentially) structurally valid x86 instruction
 * is 28 bytes:
 *  4 byte legacy prefix
 *  1 byte prefix
 *  5 byte opcode (including VEX/XOP/EVEX prefix)
 *  1 byte ModR/M
 *  1 byte SIB
 *  8 byte displacement
 *  8 byte immediate
 *
 * We want to be able to "slide" around inside of a structurally valid
 * instruction in order to find errors, so we give ourselves enough space
 * here.
 */
typedef struct {
  uint8_t off;
  uint8_t len;
  uint8_t insn[28];
} insn_candidate;

void mutator_init();
bool candidate(input_slot *slot);
