#include <llvm-c/Disassembler.h>
#include <llvm-c/Target.h>

#include "../worker.h"

static LLVMDisasmContextRef dis;

char *worker_name = "llvm";

void worker_ctor() {
  LLVMInitializeX86TargetInfo();
  LLVMInitializeX86Target();
  LLVMInitializeX86TargetMC();
  LLVMInitializeX86Disassembler();
  dis = LLVMCreateDisasm("x86_64-linux-gnu", NULL, 0, NULL, NULL);
  if (!dis) {
    errx(1, "LLVMCreateDisasm");
  }
  // Hex immediates and Intel syntax
  // The first option doesn't seem to have an effect, though.
  if (!LLVMSetDisasmOptions(dis, LLVMDisassembler_Option_PrintImmHex |
                                     LLVMDisassembler_Option_AsmPrinterVariant))
    errx(1, "LLVMSetDisasmOptions");
}

void worker_dtor() {
  LLVMDisasmDispose(dis);
}

void try_decode(decode_result *result, uint8_t *raw_insn, uint8_t length) {
  size_t len = LLVMDisasmInstruction(dis, raw_insn, length, 0, result->result, MISHEGOS_DEC_MAXLEN);
  if (len > 0) {
    result->status = S_SUCCESS;
    result->len = strlen(result->result);
    result->ndecoded = len;
  } else {
    result->status = S_FAILURE;
  }
}
