#include "common.hpp"
#include <sys/stat.h>
#include <fcntl.h>

static long sys_faccessat2(int /*fd*/, const char* /*path*/, mode_t, int /*flag*/)
{
  // TODO Same as access(), but path is relative to fd
  return -EROFS;
}

extern "C"
long syscall_SYS_faccessat2(int fd, const char *path, mode_t mode, int flag) {
  return strace(sys_faccessat2, "faccessat2", fd, path, mode, flag);
}
