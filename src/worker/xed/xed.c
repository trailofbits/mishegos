#include <xed/xed-interface.h>

#include "../worker.h"

char *worker_name = "xed";

void worker_ctor() {
  xed_tables_init();
}

void worker_dtor() {
}

decode_result *try_decode(uint8_t *raw_insn, uint8_t length, decoder_mode mode) {
  xed_decoded_inst_t xedd;
  xed_decoded_inst_zero(&xedd);
  xed_decoded_inst_set_mode(&xedd, XED_MACHINE_MODE_LONG_64, XED_ADDRESS_WIDTH_64b);

  decode_result *result = malloc(sizeof(decode_result));
  memset(result, 0, sizeof(decode_result));

  /* TODO(ww): Support mode == D_MULTIPLE.
   */
  xed_error_enum_t xed_error = xed_decode(&xedd, raw_insn, length);
  if (xed_error != XED_ERROR_NONE) {
    DLOG("xed_decode failed: %s", xed_error_enum_t2str(xed_error));

    /* Special-case XED_ERROR_BUFFER_TOO_SHORT, since it's something
     * we have a status for beyond generic failure.
     */
    if (xed_error == XED_ERROR_BUFFER_TOO_SHORT) {
      result->status = S_PARTIAL;
    } else {
      result->status = S_FAILURE;
    }
    return result;
  }

  /* TODO(ww): Fixure out whether xed_format_context decodes up to MISHEGOS_DEC_MAXLEN,
   * or saves space for the NULL terminator. It probably doesn't matter in either case
   * since nothing will be nearly that long, but it'd be good to know.
   */
  if (!xed_format_context(XED_SYNTAX_INTEL, &xedd, result->result, MISHEGOS_DEC_MAXLEN, 0, 0, 0)) {
    DLOG("xed_format_context failed!");
    /* TODO(ww): Maybe distinguish this formatting failure from the decoding
     * failure above.
     */
    result->status = S_FAILURE;
    return result;
  }

  result->status = S_SUCCESS;
  result->len = strlen(result->result);
  result->ndecoded = xed_decoded_inst_get_length(&xedd);

  return result;
}
