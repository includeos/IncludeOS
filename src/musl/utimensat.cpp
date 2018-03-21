#include "common.hpp"
#include <fcntl.h> /* Definition of AT_* constants */
#include <sys/stat.h>
#include <sys/time.h>

static int sys_utimensat(int /*dirfd*/, const char* /*path*/,
                         const struct timespec[2], int /*flags*/)
{
  // currently makes no sense, especially since we're read-only
  return -EROFS;
}

// Obsolete, use utimensat
static int sys_futimesat(int dirfd, const char* path,
                         const struct timespec times[2])
{
  return sys_utimensat(dirfd, path, times, 0);
}

static int sys_utimes(const char* /*path*/, const struct timeval[2])
{
  // currently makes no sense, especially since we're read-only
  return -EROFS;
}

extern "C"
int syscall_SYS_utimensat(int dirfd, const char *pathname,
  const struct timespec times[2], int flags)
{
  return strace(sys_utimensat, "utimensat", dirfd, pathname, times, flags);
}

extern "C"
int syscall_SYS_futimesat(int dirfd, const char *pathname,
  const struct timespec times[2])
{
  return strace(sys_futimesat, "futimesat", dirfd, pathname, times);
}

extern "C"
int syscall_SYS_utimes(const char *pathname, const struct timeval times[2]) {
  return strace(sys_utimes, "utimes", pathname, times);
}
