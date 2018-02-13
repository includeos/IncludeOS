#include "common.hpp"

static int sys_fcntl(int fd, int cmd, ...){
  return 0;
}

extern "C"
int syscall_SYS_fcntl(int fd, int cmd, ... /* arg */ )
{
  return strace(sys_fcntl, "fcntl", fd, cmd);
}
