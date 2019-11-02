#include <dr_api.h>

#include "../worker.h"

char *worker_name = "dynamorio";

void worker_ctor() {
  disassemble_set_syntax(DR_DISASM_INTEL);
}

void try_decode(decode_result *result, uint8_t *raw_insn, uint8_t length) {
  instr_t instr;
  instr_init(GLOBAL_DCONTEXT, &instr);
  uint8_t *next_pc = decode(GLOBAL_DCONTEXT, raw_insn, &instr);
  if (next_pc == NULL) {
    DLOG("dr decode failed");
    result->status = S_FAILURE;
    return;
  }

  size_t len =
      instr_disassemble_to_buffer(GLOBAL_DCONTEXT, &instr, result->result, MISHEGOS_DEC_MAXLEN);
  instr_free(GLOBAL_DCONTEXT, &instr);
  result->status = S_SUCCESS;
  result->len = len;
  result->ndecoded = next_pc - raw_insn;
}
