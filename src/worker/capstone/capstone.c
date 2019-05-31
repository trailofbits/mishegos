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

void try_decode(decode_result *result, uint8_t *raw_insn, uint8_t length, decoder_mode mode) {
  cs_insn *insn;
  size_t dec_count = mode == D_SINGLE ? 1 : 0;
  size_t count = cs_disasm(cs_hnd, raw_insn, length, 0, dec_count, &insn);
  if (count > 0) {
    result->status = S_SUCCESS;

    size_t off = 0;
    size_t pc = 0;
    for (size_t i = 0; i < count; ++i) {
      assert(off < MISHEGOS_DEC_MAXLEN);
      off += snprintf(result->result + off, MISHEGOS_DEC_MAXLEN - off, "%s %s\n", insn[i].mnemonic,
                      insn[i].op_str);
      pc += insn[i].size;
    }
    result->len = off;
    result->ndecoded = pc;

    cs_free(insn, count);
  } else {
    result->status = S_FAILURE;
  }
}
