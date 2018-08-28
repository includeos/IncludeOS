#include "common.hpp"
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>

#include <posix/fd_map.hpp>

static off_t sys_lseek(int fd, off_t offset, int whence)
{
  if(auto* fildes = FD_map::_get(fd); fildes)
    return fildes->lseek(offset, whence);

  return -EBADF;
}

static off_t sys__llseek(unsigned int /*fd*/, unsigned long /*offset_high*/,
                  unsigned long /*offset_low*/, loff_t* /*result*/,
                  unsigned int /*whence*/) {
  return -ENOSYS;
}


extern "C"
off_t syscall_SYS_lseek(int fd, off_t offset, int whence) {
  return strace(sys_lseek, "lseek", fd, offset, whence);
}

extern "C"
off_t syscall_SYS__llseek(unsigned int fd, unsigned long offset_high,
                          unsigned long offset_low, loff_t *result,
                          unsigned int whence) {
  return strace(sys__llseek, "_llseek", fd, offset_high, offset_low,
                result, whence);
}
