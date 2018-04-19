#include "common.hpp"
#include <sys/stat.h>

#include <posix/fd_map.hpp>

static long sys_fstat(int fd, struct stat* stat_buf)
{
  if (UNLIKELY(stat_buf == nullptr))
    return -EINVAL;

  if(auto* fildes = FD_map::_get(fd); fildes)
    return fildes->fstat(stat_buf);

  return -EBADF;
}

extern "C"
long syscall_SYS_fstat(int fd, struct stat* stat_buf) {
  return strace(sys_fstat, "fstat", fd, stat_buf);
}

