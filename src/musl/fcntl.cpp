#include "common.hpp"

extern "C"
int syscall_SYS_fcntl(int fd, int cmd, ... /* arg */ )
{
  STRACE("fcntl(%d, %x) = 0\n", fd, cmd);
  return 0;
}
