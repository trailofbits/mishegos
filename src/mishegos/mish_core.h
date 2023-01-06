#pragma once

#include "mish_common.h"
#include "mutator.h"

#include <stddef.h>
#include <stdint.h>

/* A wrapper around either glibc's getrandom or syscall(...),
 * if the host doesn't have a new enough glibc.
 */
int mish_getrandom(void *buf, size_t buflen, unsigned int flags);

/* Converts a hex-string (e.g., 00ffaabb) into its constituent raw bytes.
 * Assumes that outbuf is at least input_len / 2 bytes.
 */
void hex2bytes(uint8_t *outbuf, const char *const input, size_t input_len);
