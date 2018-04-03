#include "common.hpp"
#include <posix/fd_map.hpp>

static long sys_close(int fd)
{
  try {
    auto& fildes = FD_map::_get(fd);
    int res = fildes.close();
    FD_map::close(fd);
    return res;
  }
  catch(const FD_not_found&) {
    return -EBADF;
  }
}

extern "C"
int syscall_SYS_close(int fd) {
  return strace(sys_close, "close", fd);
}
