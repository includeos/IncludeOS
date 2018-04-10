#include "common.hpp"
#include <posix/fd_map.hpp>

static long sys_close(int fd)
{
  if(auto* fildes = FD_map::_get(fd); fildes)
  {
    long res = fildes->close();
    FD_map::close(fd);
    return res;
  }
  return -EBADF;
}

extern "C"
long syscall_SYS_close(int fd) {
  return strace(sys_close, "close", fd);
}
