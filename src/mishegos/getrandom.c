#define _GNU_SOURCE 1
#include <sys/types.h>
#include <unistd.h>

#if defined __GLIBC__ && defined __linux__

#if __GLIBC__ > 2 && __GLIBC_MINOR__ > 24

#include <sys/random.h>
int mish_getrandom(void *buf, size_t buflen, unsigned int flags) {
  return getrandom(buf, buflen, flags);
}
#else

#include <sys/syscall.h>
int mish_getrandom(void *buf, size_t buflen, unsigned int flags) {
  return syscall(SYS_getrandom, buf, buflen, flags);
}
#endif

#else
#error "we only support linux + glibc at the moment; help us out!"
#endif
