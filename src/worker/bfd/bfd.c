#include <dis-asm.h>
#include <stdarg.h>

#include "../worker.h"

/* BFD (libopcodes) adapter for mishegos.
 *
 * Some notes:
 * 1. libopcodes is (almost) completely undocumented. As such, a lot of the calls
 *    here are educated guesses + me reading the header file + reading the objdump
 *    source.
 * 2. This code is terrible. My original idea was to use a memfd to slurp the data
 *    coming from the fprintf callback, but didn't work for reasons.
 *    So, I switched it to a big old buffer + (v)snprintf, and that
 *    seems to mostly work. libopcodes isn't nice enough to print newlines
 *    after each instruction for us, so I do that manually.
 */

static char disasm_buf[MISHEGOS_DEC_MAXLEN];
static size_t disasm_off;

static disassembler_ftype disasm;
static struct disassemble_info disasm_info;

char *worker_name = "bfd";

static int dis_fprintf(void *_stream, const char *fmt, ...) {
  assert(disasm_off <= MISHEGOS_DEC_MAXLEN && "disassembly buffer overrun?");

  size_t remaining_size = MISHEGOS_DEC_MAXLEN - disasm_off;
  assert(remaining_size > 0);

  va_list arg;
  va_start(arg, fmt);
  size_t bytes_written = vsnprintf(disasm_buf + disasm_off, remaining_size, fmt, arg);
  disasm_off += bytes_written;
  va_end(arg);
  return 0;
}

static void init_dis() {
  disasm = disassembler(bfd_arch_i386, false, bfd_mach_x86_64, NULL);
  if (disasm == NULL) {
    errx(1, "disassembler creation failed");
  }
}

void worker_ctor() {
  init_dis();
}

decode_result *try_decode(uint8_t *raw_insn, uint8_t length, decoder_mode mode) {
  size_t pc = 0;
  size_t insn_count = 0;

  /* dis_fprintf doesn't actually use the stream argument, it just takes
   * disasm_buf from the module scope.
   *
   * I'm pretty sure most of this setup could go in init_dis, but it's here
   * because I ran into problems with the original memfd implementation.
   * Worth re-trying later.
   */
  init_disassemble_info(&disasm_info, NULL, dis_fprintf);
  disasm_info.disassembler_options = "intel-mnemonic";
  disasm_info.arch = bfd_arch_i386;
  disasm_info.mach = bfd_mach_x86_64;
  disasm_info.read_memory_func = buffer_read_memory;
  disasm_info.buffer = raw_insn;
  disasm_info.buffer_vma = 0;
  disasm_info.buffer_length = length;
  disassemble_init_for_target(&disasm_info);

  memset(disasm_buf, 0, MISHEGOS_DEC_MAXLEN);

  size_t insn_size;
  disasm_off = 0;
  do {
    insn_size = disasm(pc, &disasm_info);

    /* Make sure each instruction is on its own line in the disassembly buffer.
     */
    size_t nl = snprintf(disasm_buf + disasm_off, MISHEGOS_DEC_MAXLEN - disasm_off, "\n");
    assert(nl == 1 && "should have written exactly one byte");
    _unused(nl);
    disasm_off++;

    pc += insn_size;
    insn_count++;
  } while (insn_size > 0 && pc < length && mode != D_SINGLE);

  decode_result *result = malloc(sizeof(decode_result));
  memset(result, 0, sizeof(decode_result));

  if (pc <= 0 || insn_count == 0 || strstr(disasm_buf, "(bad)") != NULL) {
    result->status = S_FAILURE;
  } else {
    result->status = S_SUCCESS;
  }

  memcpy(result->result, disasm_buf, disasm_off);
  result->len = disasm_off;
  result->ndecoded = pc;

  return result;
}
