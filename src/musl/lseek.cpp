#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>

#include "stub.hpp"

off_t sys_lseek(int fd, off_t offset, int whence) {
  errno = ENOSYS;
  return -1;
}

off_t sys__llseek(unsigned int fd, unsigned long offset_high,
                  unsigned long offset_low, loff_t *result,
                  unsigned int whence) {
  errno = ENOSYS;
  return -1;
}


extern "C"
off_t syscall_SYS_lseek(int fd, off_t offset, int whence) {
  return stubtrace(sys_lseek, "lseek", fd, offset, whence);
}

extern "C"
int syscall_SYS__llseek(unsigned int fd, unsigned long offset_high,
                          unsigned long offset_low, loff_t *result,
                          unsigned int whence) {
  return stubtrace(sys__llseek, "_llseek", fd, offset_high, offset_low,
                   result, whence);
}
