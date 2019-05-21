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

decode_result *try_decode(uint8_t *raw_insn, uint8_t length) {
  cs_insn *insn;
  size_t count;

  decode_result *result = malloc(sizeof(decode_result));
  memset(result, 0, sizeof(decode_result));

  count = cs_disasm(cs_hnd, raw_insn, length, 0, 0, &insn);
  if (count > 0) {
    result->status = S_SUCCESS;

    size_t off = 0;
    for (size_t i = 0; i < count; ++i) {
      assert(off < MISHEGOS_DEC_MAXLEN);
      off += snprintf(result->result + off, MISHEGOS_DEC_MAXLEN - off,
                      "%s %s\n", insn[i].mnemonic, insn[i].op_str);
    }
    result->len = off;

    cs_free(insn, count);
  } else {
    result->status = S_FAILURE;
  }

  return result;
}
