#pragma once

#ifdef DEBUG
#define DLOG(fmt, ...)                                                                             \
  fprintf(stderr, "%s:%d %s: " fmt "\n", __FILE__, __LINE__, __func__, ##__VA_ARGS__);
#undef NDEBUG
#define _unused(x)
#define hexputs(x, y) _hexputs(x, y)
#else
#define DLOG(...)
#define NDEBUG
#define _unused(x) ((void)(x))
#define hexputs(x, y)                                                                              \
  _unused(x);                                                                                      \
  _unused(y)
#endif

#include <assert.h>
#include <errno.h>
#include <err.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <stdio.h>
#include <stdbool.h>
#include <fcntl.h>
#include <semaphore.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/stat.h>
#include <dlfcn.h>
#include <sys/wait.h>
#include <setjmp.h>
#include <time.h>

#define VERBOSE(fmt, ...)                                                                          \
  if (verbose) {                                                                                   \
    fprintf(stderr, fmt "\n", ##__VA_ARGS__);                                                      \
  }

#define MISHEGOS_INSN_MAXLEN 15
#define MISHEGOS_DEC_MAXLEN 1018

#define MISHEGOS_IN_NSLOTS 10 // TODO(ww): Increase?
static_assert(MISHEGOS_IN_NSLOTS >= 2, "MISHEGOS_IN_NSLOTS should be >= 2");
#define MISHEGOS_OUT_NSLOTS 10 // TODO(ww): Increase?

/* Size of our worker bitmask, minus 1 (to avoid UB).
 */
#define MISHEGOS_MAX_NWORKERS 31
static_assert(MISHEGOS_MAX_NWORKERS == 31, "MISHEGOS_MAX_NWORKERS cannot exceed 31");
#define MISHEGOS_COHORT_NSLOTS 60
/* NOTE(ww): If this seems a little weird, remember that there are up to
 * MISHEGOS_IN_NSLOTS + MISHEGOS_OUT_NSLOTS outputs contesting for addition to
 * a cohort. Without enough cohorts to make every in-flight output happy, we
 * end up with a deadlock.
 */
static_assert(MISHEGOS_COHORT_NSLOTS >= MISHEGOS_IN_NSLOTS + MISHEGOS_OUT_NSLOTS,
              "MISHEGOS_COHORT_NSLOTS should be >= MISHEGOS_IN_NSLOTS + MISHEGOS_OUT_NSLOTS");
#define MISHEGOS_COHORT_SEMFMT "/mishegos_csem%d"
#define MISHEGOS_IN_SEMFMT "/mishegos_isem%d"
#define MISHEGOS_OUT_SEMFMT "/mishegos_osem%d"

#define MISHEGOS_SHMNAME "/mishegos_shm"
#define MISHEGOS_SHMSIZE                                                                           \
  ((sizeof(mishegos_config)) + (MISHEGOS_IN_NSLOTS * sizeof(input_slot)) +                         \
   (MISHEGOS_OUT_NSLOTS * sizeof(output_slot)))

#define GET_I_SEM(semno)                                                                           \
  (((mishegos_isems) && ((semno) < MISHEGOS_IN_NSLOTS)) ? mishegos_isems[slotno] : NULL)

#define GET_CONFIG() ((mishegos_config *)(mishegos_arena))

#define GET_I_SLOT(slotno)                                                                         \
  (((mishegos_arena) && ((slotno) < MISHEGOS_IN_NSLOTS))                                           \
       ? (input_slot *)(mishegos_arena + (sizeof(mishegos_config)) +                               \
                        (sizeof(input_slot) * slotno))                                             \
       : NULL)

#define GET_O_SLOT(slotno)                                                                         \
  (((mishegos_arena) && ((slotno) < MISHEGOS_OUT_NSLOTS))                                          \
       ? (output_slot *)(mishegos_arena + (sizeof(mishegos_config)) +                              \
                         (sizeof(input_slot) * MISHEGOS_IN_NSLOTS) +                               \
                         (sizeof(output_slot) * slotno))                                           \
       : NULL)

typedef enum {
  S_NONE = 0,
  S_SUCCESS,
  S_FAILURE,
  S_CRASH,
  S_PARTIAL,
  S_UNKNOWN,
} decode_status;

typedef enum {
  M_HAVOC = 0,
  M_SLIDING,
  M_STRUCTURED,
  M_DUMMY,
  M_MANUAL,
} mutator_mode;

typedef enum {
  W_IGNORE_CRASHES,
} worker_config_mask;

typedef struct {
  char *so;
  pid_t pid;
  bool running;
} worker;

typedef struct __attribute__((packed)) {
  uint32_t nworkers;
  uint32_t worker_config;
  uint64_t rng_seed[4];
  mutator_mode mut_mode;
} mishegos_config;
static_assert(sizeof(mishegos_config) == 44, "mishegos_config should be 44 bytes");

typedef struct __attribute__((packed)) {
  uint32_t workers;
  uint8_t len;
  uint8_t raw_insn[MISHEGOS_INSN_MAXLEN];
} input_slot;
static_assert(sizeof(input_slot) == 20, "input_slot should be 20 bytes");

typedef struct __attribute__((packed)) {
  input_slot input;
  decode_status status;
  uint16_t len;
  char result[MISHEGOS_DEC_MAXLEN];
  uint16_t ndecoded;
  uint32_t workerno;
} output_slot;
static_assert(sizeof(output_slot) == 1050, "output_slot should be 1050 bytes");

typedef struct {
  uint32_t workers;
  output_slot *outputs;
} output_cohort;

static inline void _hexputs(uint8_t *buf, uint8_t len) {
  for (int i = 0; i < len; ++i) {
    fprintf(stderr, "%02x", buf[i]);
  }
  fprintf(stderr, "\n");
}

static inline const char *status2str(decode_status status) {
  switch (status) {
  case S_NONE:
    return "none";
  case S_SUCCESS:
    return "success";
  case S_FAILURE:
    return "failure";
  case S_CRASH:
    return "crash";
  case S_PARTIAL:
    return "partial";
  case S_UNKNOWN:
  default:
    return "unknown";
  }
}
