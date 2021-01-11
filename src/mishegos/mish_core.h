#pragma once

#include "mish_common.h"
#include "mutator.h"
#include "cohorts.h"

extern uint8_t *mishegos_arena;

/* A wrapper around either glibc's getrandom or syscall(...),
 * if the host doesn't have a new enough glibc.
 */
int mish_getrandom(void *buf, size_t buflen, unsigned int flags);

const char *get_worker_so(uint32_t workerno);

/* Returns a hex-string representation of the given input slot.
 * The returned string should be freed.
 */
char *hexdump(input_slot *slot);

/* Converts a hex-string (e.g., 00ffaabb) into its constituent raw bytes.
 * Assumes that outbuf is at least input_len / 2 bytes.
 */
void hex2bytes(uint8_t *outbuf, const char *const input, size_t input_len);

/* Sleeps the current thread for the given number of milliseconds.
 */
void millisleep(uint64_t millis);
