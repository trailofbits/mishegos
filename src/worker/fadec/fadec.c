#include <fadec.h>

#include "../worker.h"

char *worker_name = "fadec";

void try_decode(decode_result *result, uint8_t *raw_insn, uint8_t length) {
  FdInstr inst;
  int r = fd_decode(raw_insn, length, 64, 0, &inst);
  if (r > 0) {
    result->status = S_SUCCESS;
    fd_format(&inst, result->result, MISHEGOS_DEC_MAXLEN);
    result->len = strlen(result->result);
    result->ndecoded = FD_SIZE(&inst);
  } else {
    result->status = r == FD_ERR_PARTIAL ? S_PARTIAL : S_FAILURE;
  }
}
