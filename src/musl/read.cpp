#include "common.hpp"
#include <posix/fd_map.hpp>

static long sys_read(int fd, void* buf, size_t len)
{
  if(UNLIKELY(buf == nullptr))
    return -EFAULT;

  if(auto* fildes = FD_map::_get(fd); fildes)
    return fildes->read(buf, len);

  return -EBADF;
}

extern "C"
long syscall_SYS_read(int fd, void *buf, size_t nbyte) {
  return strace(sys_read, "read", fd, buf, nbyte);
}
