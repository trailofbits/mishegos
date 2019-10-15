#include <udis86.h>

#include "../worker.h"

static ud_t udis;

char *worker_name = "udis86";

void worker_ctor() {
  ud_init(&udis);
  ud_set_mode(&udis, 64);
  ud_set_syntax(&udis, UD_SYN_INTEL);
  ud_set_vendor(&udis, UD_VENDOR_ANY);
}

void try_decode(decode_result *result, uint8_t *raw_insn, uint8_t length) {
  ud_set_input_buffer(&udis, raw_insn, length);

  size_t count = ud_disassemble(&udis);
  if (count <= 0 || strstr(ud_insn_asm(&udis), "invalid")) {
    result->status = S_FAILURE;
  } else {
    result->status = S_SUCCESS;
    strncpy(result->result, ud_insn_asm(&udis), MISHEGOS_DEC_MAXLEN);
    result->len = strlen(result->result);
    result->ndecoded = ud_insn_len(&udis);
  }
}
