#include "common.hpp"
#include <posix/fd_map.hpp>

static long sys_fcntl(int fd, int cmd, va_list va) {
  try {
    return FD_map::_get(fd).fcntl(cmd, va);
  }
  catch(const FD_not_found&) {
    return -EBADF;
  }
}

extern "C"
long syscall_SYS_fcntl(int fd, int cmd, ... /* arg */ )
{
  va_list va;
  va_start(va, cmd);
  auto ret = strace(sys_fcntl, "fcntl", fd, cmd, va);
  va_end(va);
  return ret;
}
