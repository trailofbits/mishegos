#include <Zydis/Zydis.h>

#include "../worker.h"

char *worker_name = "zydis";

static ZydisDecoder zdecoder;
static ZydisFormatter zformatter;

/* I couldn't find this defined anywhere in zycore/zydis.
 */
static const char *ZyanStatus_strerror(ZyanStatus zstatus) {
  switch (zstatus) {
  case ZYDIS_STATUS_NO_MORE_DATA: {
    return "no more data";
  }
  case ZYDIS_STATUS_DECODING_ERROR: {
    return "general decoding error";
  }
  case ZYDIS_STATUS_INSTRUCTION_TOO_LONG: {
    return "instruction too long";
  }
  case ZYDIS_STATUS_BAD_REGISTER: {
    return "invalid register";
  }
  case ZYDIS_STATUS_ILLEGAL_LOCK: {
    return "illegal lock prefix";
  }
  case ZYDIS_STATUS_ILLEGAL_LEGACY_PFX: {
    return "illegal legacy prefix";
  }
  case ZYDIS_STATUS_ILLEGAL_REX: {
    return "illegal REX prefix";
  }
  case ZYDIS_STATUS_INVALID_MAP: {
    return "illegal opcode map value";
  }
  case ZYDIS_STATUS_MALFORMED_EVEX: {
    return "illegal EVEX prefix";
  }
  case ZYDIS_STATUS_MALFORMED_MVEX: {
    return "illegal MVEX prefix";
  }
  case ZYDIS_STATUS_INVALID_MASK: {
    return "invalid write mask";
  }
  default: {
    return "unknown";
  }
  }
}

void worker_ctor() {
  ZydisDecoderInit(&zdecoder, ZYDIS_MACHINE_MODE_LONG_64, ZYDIS_ADDRESS_WIDTH_64);
  ZydisFormatterInit(&zformatter, ZYDIS_FORMATTER_STYLE_INTEL);

  /* TODO(ww): Zydis has a bunch of formatter options; we probably
   * want to set some of them to make its output easier to normalize.
   */
}

void try_decode(decode_result *result, uint8_t *raw_insn, uint8_t length) {
  _unused(ZyanStatus_strerror);

  ZydisDecodedInstruction insn;
  ZyanStatus zstatus = ZydisDecoderDecodeBuffer(&zdecoder, raw_insn, length, &insn);
  if (!ZYAN_SUCCESS(zstatus)) {
    DLOG("zydis decoding failed: %s", ZyanStatus_strerror(zstatus));

    if (zstatus == ZYDIS_STATUS_NO_MORE_DATA) {
      result->status = S_PARTIAL;
    } else {
      result->status = S_FAILURE;
    }
    return;
  }

  zstatus =
      ZydisFormatterFormatInstruction(&zformatter, &insn, result->result, MISHEGOS_DEC_MAXLEN, 0);
  if (!ZYAN_SUCCESS(zstatus)) {
    DLOG("zydis formatting failed: %s", ZyanStatus_strerror(zstatus));
    result->status = S_FAILURE;
    return;
  }

  result->status = S_SUCCESS;
  result->len = strlen(result->result);
  result->ndecoded = insn.length;
}
