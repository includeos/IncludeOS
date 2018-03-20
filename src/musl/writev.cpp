#include "common.hpp"

#include <kprint>
#include <sys/uio.h>

static ssize_t sys_writev(int fd, const struct iovec *iov, int iovcnt)
{
  if(fd <= 2)
  {
    ssize_t res = 0;
    for(int i = 0; i < iovcnt; i++)
    {
      auto* text = (const char*)iov[i].iov_base;
      auto len = iov[i].iov_len;
      OS::print(text, len);
      res += len;
    }
    return res;
  }
  return 0;
}

extern "C"
ssize_t syscall_SYS_writev(int fd, const struct iovec *iov, int iovcnt){
  //return strace(sys_writev, "writev", fd, iov, iovcnt);
  return sys_writev(fd, iov, iovcnt);
}
