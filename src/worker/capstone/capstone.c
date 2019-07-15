#include <capstone/capstone.h>

#include "../worker.h"

static csh cs_hnd;

char *worker_name = "capstone";

void worker_ctor() {
  if (cs_open(CS_ARCH_X86, CS_MODE_64, &cs_hnd) != CS_ERR_OK) {
    errx(1, "cs_open");
  }
}

void worker_dtor() {
  cs_close(&cs_hnd);
}

void try_decode(decode_result *result, uint8_t *raw_insn, uint8_t length) {
  cs_insn *insn;
  size_t count = cs_disasm(cs_hnd, raw_insn, length, 0, 1, &insn);
  if (count > 0) {
    result->status = S_SUCCESS;
    result->len =
        snprintf(result->result, MISHEGOS_DEC_MAXLEN, "%s %s\n", insn[0].mnemonic, insn[0].op_str);
    result->ndecoded = insn[0].size;

    cs_free(insn, count);
  } else {
    result->status = S_FAILURE;
  }
}
