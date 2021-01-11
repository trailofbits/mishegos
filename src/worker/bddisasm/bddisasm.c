#include "bddisasm/inc/disasmtypes.h"
#include "bddisasm/inc/bddisasm.h"
#include "../worker.h"

char *worker_name = "bddisasm";

/* This is used by NdToText. */
int nd_vsnprintf_s(char *buffer, size_t sizeOfBuffer, size_t count, const char *format,
                   va_list argptr) {
  return vsnprintf(buffer, sizeOfBuffer, format, argptr);
}

/* This is used by NdDecode. */
void *nd_memset(void *s, int c, size_t n) {
  return memset(s, c, n);
}

static const char *bddisasm_strerror(NDSTATUS ndstatus) {
  switch (ndstatus) {
  case ND_STATUS_BUFFER_TOO_SMALL: {
    return "The provided input buffer is too small.";
  }
  case ND_STATUS_INVALID_ENCODING: {
    return "Invalid encoding/instruction.";
  }
  case ND_STATUS_INSTRUCTION_TOO_LONG: {
    return "Instruction exceeds the maximum 15 bytes.";
  }
  case ND_STATUS_INVALID_PREFIX_SEQUENCE: {
    return "Invalid prefix sequence is present.";
  }
  case ND_STATUS_INVALID_REGISTER_IN_INSTRUCTION: {
    return "The instruction uses an invalid register.";
  }
  case ND_STATUS_XOP_WITH_PREFIX: {
    return "XOP is present, but also a legacy prefix.";
  }
  case ND_STATUS_VEX_WITH_PREFIX: {
    return "VEX is present, but also a legacy prefix.";
  }
  case ND_STATUS_EVEX_WITH_PREFIX: {
    return "EVEX is present, but also a legacy prefix.";
  }
  case ND_STATUS_INVALID_ENCODING_IN_MODE: {
    return "Invalid encoding/instruction.";
  }
  case ND_STATUS_BAD_LOCK_PREFIX: {
    return "Invalid usage of LOCK.";
  }
  case ND_STATUS_CS_LOAD: {
    return "An attempt to load the CS register.";
  }
  case ND_STATUS_66_NOT_ACCEPTED: {
    return "0x66 prefix is not accepted.";
  }
  case ND_STATUS_16_BIT_ADDRESSING_NOT_SUPPORTED: {
    return "16 bit addressing mode not supported.";
  }
  case ND_STATUS_RIP_REL_ADDRESSING_NOT_SUPPORTED: {
    return "RIP-relative addressing not supported.";
  }
  case ND_STATUS_VSIB_WITHOUT_SIB: {
    return "Instruction uses VSIB, but SIB is not present.";
  }
  case ND_STATUS_INVALID_VSIB_REGS: {
    return "VSIB addressing, same vector reg used more than once.";
  }
  case ND_STATUS_VEX_VVVV_MUST_BE_ZERO: {
    return "VEX.VVVV field must be zero.";
  }
  case ND_STATUS_MASK_NOT_SUPPORTED: {
    return "Masking is not supported.";
  }
  case ND_STATUS_MASK_REQUIRED: {
    return "Masking is mandatory.";
  }
  case ND_STATUS_ER_SAE_NOT_SUPPORTED: {
    return "Embedded rounding/SAE not supported.";
  }
  case ND_STATUS_ZEROING_NOT_SUPPORTED: {
    return "Zeroing not supported.";
  }
  case ND_STATUS_ZEROING_ON_MEMORY: {
    return "Zeroing on memory.";
  }
  case ND_STATUS_ZEROING_NO_MASK: {
    return "Zeroing without masking.";
  }
  case ND_STATUS_BROADCAST_NOT_SUPPORTED: {
    return "Broadcast not supported.";
  }
  case ND_STATUS_INVALID_PARAMETER: {
    return "An invalid parameter was provided.";
  }
  case ND_STATUS_INVALID_INSTRUX: {
    return "The INSTRUX contains unexpected values.";
  }
  case ND_STATUS_BUFFER_OVERFLOW: {
    return "Not enough space is available to format textual disasm.";
  }
  case ND_STATUS_INTERNAL_ERROR: {
    return "Internal error occurred.";
  }
  default: {
    return "unknown";
  }
  }
}

/* NOTE(ww): AFAICT, there's no easy way to flag these off in bddissasm. */
static inline decode_status via_or_cyrix_weirdness(const INSTRUX *instr) {
  switch (instr->Instruction) {
  case ND_INS_ALTINST:
  case ND_INS_CPU_READ:
  case ND_INS_CPU_WRITE:
    return S_UNKNOWN; /* feels more appropriate than S_FAILURE */
  default:
    return S_SUCCESS;
  }
}

void try_decode(decode_result *result, uint8_t *raw_insn, uint8_t length) {
  _unused(bddisasm_strerror);

  INSTRUX instruction;
  NDSTATUS ndstatus;

  ndstatus = NdDecodeEx(&instruction, raw_insn, length, ND_CODE_64, ND_DATA_64);
  if (!ND_SUCCESS(ndstatus)) {
    DLOG("bddisasm decoding failed: %s", bddisasm_strerror(ndstatus));
    result->status = S_FAILURE;
    return;
  }

  ndstatus = NdToText(&instruction, 0, sizeof(result->result), result->result);
  if (!ND_SUCCESS(ndstatus)) {
    DLOG("bddisasm formatting failed: %s", bddisasm_strerror(ndstatus));
    result->status = S_FAILURE;
    return;
  }

  /* Preserve the successful decoding, but mark it as a failure if it's something
   * from a weird x86 vendor (like Cyrix or VIA).
   */
  result->status = via_or_cyrix_weirdness(&instruction);
  result->len = strlen(result->result);
  result->ndecoded = instruction.Length;
}
