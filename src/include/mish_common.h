#pragma once

#ifdef DEBUG
#define DLOG(fmt, ...)                                                                             \
  fprintf(stderr, "%s:%d %s: " fmt "\n", __FILE__, __LINE__, __func__, ##__VA_ARGS__);
#undef NDEBUG
#define _unused(x)
#else
#define DLOG(...)
#define NDEBUG
#define _unused(x) ((void) (x))
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

#define MISHEGOS_INSN_MAXLEN 15
#define MISHEGOS_DEC_MAXLEN 1018

#define MISHEGOS_IN_NSLOTS 2
#define MISHEGOS_OUT_NSLOTS 1
#define MISHEGOS_NWORKERS 2
#define MISHEGOS_MAX_NWORKERS 31 // Size of our worker bitmask, minus 1 (to avoid UB).
#define MISHEGOS_IN_SEMFMT "/mishegos_isem%d"
#define MISHEGOS_OUT_SEMNAME "/mishegos_osem"

#define MISHEGOS_SHMNAME "/mishegos_shm"
#define MISHEGOS_SHMSIZE                                                                           \
  ((MISHEGOS_IN_NSLOTS * sizeof(input_slot)) + (MISHEGOS_OUT_NSLOTS * sizeof(output_slot)))

#define GET_I_SEM(semno)                                                                           \
  (((mishegos_isems) && ((semno) < MISHEGOS_IN_NSLOTS)) ? mishegos_isems[slotno] : NULL)

#define GET_I_SLOT(slotno)                                                                         \
  (((mishegos_arena) && ((slotno) < MISHEGOS_IN_NSLOTS))                                           \
       ? (input_slot *)(mishegos_arena + (sizeof(input_slot) * slotno))                            \
       : NULL)

#define GET_O_SLOT(slotno)                                                                         \
  (((mishegos_arena) && ((slotno) < MISHEGOS_OUT_NSLOTS))                                          \
       ? (output_slot *)(mishegos_arena + (sizeof(input_slot) * MISHEGOS_IN_NSLOTS) +              \
                         (sizeof(output_slot) * slotno))                                           \
       : NULL)

typedef enum {
  S_NONE,
  S_SUCCESS,
  S_FAILURE,
  S_CRASH,
  S_HANG,
  S_PARTIAL,
  S_WOULDBLOCK,
  S_UNKNOWN,
} decode_status;

typedef struct {
  uint32_t no;
  char *so;
  pid_t pid;
} worker;

typedef struct __attribute__((packed)) {
  uint32_t workers;
  uint8_t len;
  uint8_t raw_insn[15];
} input_slot;
static_assert(sizeof(input_slot) == 20, "input_slot should be 20 bytes");

typedef struct __attribute__((packed)) {
  input_slot input;
  decode_status status;
  uint16_t len;
  char result[MISHEGOS_DEC_MAXLEN];
} output_slot;
static_assert(sizeof(output_slot) == 1044, "output_slot should be 1044 bytes");

static inline void hexputs(uint8_t *buf, uint8_t len) {
  for (int i = 0; i < len; ++i) {
    printf("%02x", buf[i]);
  }
  printf("\n");
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
  case S_HANG:
    return "hang";
  case S_PARTIAL:
    return "partial";
  case S_WOULDBLOCK:
    return "wouldblock";
  case S_UNKNOWN:
  default:
    return "unknown";
  }
}
