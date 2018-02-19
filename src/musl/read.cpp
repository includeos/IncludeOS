#include "common.hpp"
#include <fd_map.hpp>

static ssize_t sys_read(int fd, void* buf, size_t len) {
  try {
    auto& fildes = FD_map::_get(fd);
    return fildes.read(buf, len);
  }
  catch(const FD_not_found&) {
    errno = EBADF;
    return -1;
  }
}

extern "C"
ssize_t syscall_SYS_read(int fd, void *buf, size_t nbyte) {
  return strace(sys_read, "read", fd, buf, nbyte);
}
