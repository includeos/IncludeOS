#include "common.hpp"
#include <sys/uio.h>

static long sys_writev(int fd, const struct iovec *iov, int iovcnt)
{
  if (fd == 1 || fd == 2)
  {
    long res = 0;
    for(int i = 0; i < iovcnt; i++)
    {
      auto* text = (const char*)iov[i].iov_base;
      auto len = iov[i].iov_len;
      os::print(text, len);
      res += len;
    }
    return res;
  }
  return 0;
}

extern "C"
long syscall_SYS_writev(int fd, const struct iovec *iov, int iovcnt){
  //return strace(sys_writev, "writev", fd, iov, iovcnt);
  return sys_writev(fd, iov, iovcnt);
}
