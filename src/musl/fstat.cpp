#include "common.hpp"
#include <sys/stat.h>

#include <posix/fd_map.hpp>

static long sys_fstat(int filedes, struct stat* stat_buf)
{
  if (UNLIKELY(stat_buf == nullptr))
    return -EINVAL;

  try {
    auto& fd = FD_map::_get(filedes);
    return fd.fstat(stat_buf);
  }
  catch(const FD_not_found&) {
    return -EBADF;
  }
}

extern "C"
long syscall_SYS_fstat(int fd, struct stat* stat_buf) {
  return strace(sys_fstat, "fstat", fd, stat_buf);
}

