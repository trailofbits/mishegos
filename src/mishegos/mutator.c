#include "mish_core.h"

static mutator_mode mut_mode;
static insn_candidate insn_cand;

/* An x86 instruction can have up to 4 legacy prefixes,
 * in any order, with no more than 1 prefix from each group.
 */
static uint8_t legacy_prefixes[] = {
    // Prefix group 1.
    0xf0, // repeat/lock
    0xf3, // rep, repe
    0xf2, // repne
    // Prefix group 2.
    0x2e, // segment override, cs
    0x36, // segment override, ss
    0x3e, // segment override, ds
    0x26, // segment override, es
    0x64, // segment override, fs
    0x65, // segment override, gs
    // Prefix group 3.
    0x66, // operand size override
    // Prefix group 4.
    0x67, // address size override
};

/* REX prefixes apply in long (64-bit) mode, and are made up
 * of a fixed 4-bit pattern + extension bits for operand size,
 * ModR/M and SIB.
 *
 * Each instruction should only have one REX prefix.
 */
static uint8_t rex_prefixes[] = {
    0b01000000, // ----
    0b01000001, // ---B
    0b01000010, // --X-
    0b01000011, // --BX
    0b01000100, // -R--
    0b01000101, // -R-B
    0b01000110, // -RX-
    0b01000111, // -RXB
    0b01001000, // W---
    0b01001001, // W--B
    0b01001010, // W-X-
    0b01001011, // W-XB
    0b01001100, // WR--
    0b01001101, // WR-B
    0b01001110, // WRX-
    0b01001111, // WRXB
};

static uint8_t rand_byte() {
  uint8_t b;
  getrandom(&b, 1, 0);
  return b;
}

/* Creates a random (potentially invalid) opcode.
 * Opcodes are 1-3 bytes long, and come in three formats:
 *  1. Single byte (raw opcode)
 *  2. Two bytes (escape byte, opcode)
 *  3. Three bytes (escape byte 1, escape byte 2, opcode)
 */
static void rand_opcode(opcode *opc) {
  switch (rand_byte() % 4) {
  case 0: {
    opc->len = 1;
    opc->op[0] = rand_byte();
    break;
  }
  case 1: {
    opc->len = 2;
    opc->op[0] = 0x0f;
    opc->op[1] = rand_byte();
    break;
  }
  case 2: {
    opc->len = 3;
    opc->op[0] = 0x0f;
    opc->op[1] = 0x38;
    opc->op[2] = rand_byte();
    break;
  }
  case 3: {
    opc->len = 3;
    opc->op[0] = 0x0f;
    opc->op[1] = 0x3a;
    opc->op[2] = rand_byte();
    break;
  }
  }
}

static void build_sliding_candidate() {
  memset(&insn_cand, 0, sizeof(insn_candidate));

  /* 4 random legacy prefixes.
   *
   * Observe that we don't attempt to enforce the "1 prefix from each group" rule.
   */
  for (int i = 0; i < 4; ++i) {
    insn_cand.insn[i] = legacy_prefixes[rand_byte() % sizeof(legacy_prefixes)];
  }
  insn_cand.len += 4;

  /* REX prefix choices:
   *   0. Random prefix from rex_prefixes table
   *   1. Completely randomized prefix
   *   3. No REX prefix
   */
  switch (rand_byte() % 3) {
  case 0: {
    insn_cand.insn[insn_cand.len] = rex_prefixes[rand_byte() % sizeof(rex_prefixes)];
    insn_cand.len++;
    break;
  }
  case 1: {
    insn_cand.insn[insn_cand.len] = rand_byte();
    insn_cand.len++;
    break;
  }
  case 2: {
    break;
  }
  }

  /* Opcode, up to 3 bytes.
   */
  opcode opc;
  rand_opcode(&opc);
  memcpy(insn_cand.insn + insn_cand.len, opc.op, opc.len);
  insn_cand.len += opc.len;

  /* ModR/M and SIB. For now, just two random bytes.
   */
  insn_cand.insn[insn_cand.len++] = rand_byte();
  insn_cand.insn[insn_cand.len++] = rand_byte();

  /* Displacement. Either none, or 1, 2, 4, or 8 bytes.
   */
  if (rand_byte() % 2 == 0) {
    uint8_t displen = 1 << (rand_byte() % 4);
    uint64_t disp;
    getrandom(&disp, displen, 0);
    memcpy(insn_cand.insn + insn_cand.len, &disp, displen);
    insn_cand.len += displen;
  }

  /* Immediate. Either none, or 1, 2, 4, or 8 bytes.
   */
  if (rand_byte() % 2 == 0) {
    uint8_t immlen = 1 << (rand_byte() % 4);
    uint64_t imm;
    getrandom(&imm, immlen, 0);
    memcpy(insn_cand.insn + insn_cand.len, &imm, immlen);
    insn_cand.len += immlen;
  }
}

/* Havoc: generate a random instruction candidate.
 */
static void havoc_candidate(input_slot *slot) {
  slot->len = (rand_byte() % MISHEGOS_INSN_MAXLEN) + 1;
  getrandom(slot->raw_insn, slot->len, 0);
}

/* Sliding: generate an instruction candidate with the
 * "sliding" approach.
 *
 * Essentially,
 */
static void sliding_candidate(input_slot *slot) {
  /* An offset of zero into our sliding candidate indicates that we've slid
   * all the way through and need to build a new candidate.
   */
  if (insn_cand.off == 0) {
    build_sliding_candidate();
  }

  /* If our sliding candidate is less than the maximum instruction size,
   * then we have nothing to slide. Just copy it try a new candidate on the next
   * call.
   *
   * Otherwise, take the maximum instruction size from our sliding
   * candidate, plus the current offset. This gives us a bunch of
   * high quality instruction "windows".
   */
  if (insn_cand.len <= MISHEGOS_INSN_MAXLEN) {
    memcpy(slot->raw_insn, insn_cand.insn, insn_cand.len);
    slot->len = insn_cand.len;
    insn_cand.off = 0; // Shouldn't be necessary, but just to be explicit.
  } else {
    memcpy(slot->raw_insn, insn_cand.insn + insn_cand.off, MISHEGOS_INSN_MAXLEN);
    slot->len = MISHEGOS_INSN_MAXLEN;
    insn_cand.off = (insn_cand.off + 1) % (insn_cand.len - MISHEGOS_INSN_MAXLEN + 1);
  }
}

/* Dummy: Generates a single NOP for debugging purposes.
 */
static void dummy_candidate(input_slot *slot) {
  slot->raw_insn[0] = 0x90;
  slot->len = 1;
}

void mutator_init() {
  mut_mode = GET_CONFIG()->mut_mode;
}

void candidate(input_slot *slot) {
  switch (mut_mode) {
  case M_HAVOC: {
    havoc_candidate(slot);
    break;
  }
  case M_SLIDING: {
    sliding_candidate(slot);
    break;
  }
  case M_DUMMY: {
    dummy_candidate(slot);
    break;
  }
  }
  return;
}
