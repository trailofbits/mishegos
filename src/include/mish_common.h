#pragma once

#ifdef DEBUG
#define DLOG(fmt, ...)                                                                             \
  fprintf(stderr, "%s:%d %s: " fmt "\n", __FILE__, __LINE__, __func__, ##__VA_ARGS__);
#undef NDEBUG
#define _unused(x)
#else
#define DLOG(...)
#define NDEBUG
#define _unused(x) ((void)(x))
#endif

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>

#define MISHEGOS_INSN_MAXLEN 15
#define MISHEGOS_DEC_MAXLEN 248
// This limit is rather arbitrary at the moment.
#define MISHEGOS_MAX_NWORKERS 31

typedef enum {
  S_NONE = 0,
  S_SUCCESS,
  S_FAILURE,
  S_CRASH,
  S_PARTIAL,
  S_UNKNOWN,
} decode_status;

typedef enum {
  W_IGNORE_CRASHES,
} worker_config_mask;

typedef struct {
  uint8_t len;
  uint8_t raw_insn[MISHEGOS_INSN_MAXLEN];
} input_slot;

typedef struct __attribute__((packed)) {
  decode_status status;
  uint16_t ndecoded;
  uint16_t len;
  char result[MISHEGOS_DEC_MAXLEN];
} output_slot;
static_assert(sizeof(output_slot) == 256, "output_slot should be 256 bytes");
