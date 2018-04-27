#include "common.hpp"
#include <dirent.h>
#include <posix/fd_map.hpp>

static long sys_getdents(unsigned int fd, struct dirent *dirp, unsigned int count)
{
  if(auto* fildes = FD_map::_get(fd); fildes)
    return fildes->getdents(dirp, count);

  return -EBADF;
}

extern "C"
long syscall_SYS_getdents(unsigned int fd, struct dirent *dirp, unsigned int count) {
  return strace(sys_getdents, "getdents", fd, dirp, count);
}
