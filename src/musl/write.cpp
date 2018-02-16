#include <os>
#include "common.hpp"

// The actual syscall
static long sys_write(int fd, char* str, size_t len) {

  if (fd <= 0) {
    errno = EBADF;
    return -1;
  }

  if (fd == 1 or fd == 2) {
    OS::print(str, len);
    return len;
  }

  // TODO: integrate with old file descriptors
  errno = ENOSYS;
  return -1;
}

// The syscall wrapper, using strace if enabled
extern "C"
long syscall_SYS_write(int fd, char* str, size_t len) {
  return strace(sys_write, "write", fd, str, len);
}
