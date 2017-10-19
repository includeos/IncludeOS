#include <sys/time.h>
#include <utime.h>
#include <errno.h>
#include <cstdio>

int utime(const char *filename, const struct utimbuf *times)
{
  printf("utime(%s, %p) called, not supported\n", filename, times);
  errno = ENOTSUP;
  return -1;
}

int utimes(const char *filename, const struct timeval times[2])
{
  printf("utimes(%s, %p) called, not supported\n", filename, times);
  errno = ENOTSUP;
  return -1;
}
