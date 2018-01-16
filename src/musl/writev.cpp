#include "common.hpp"

#include <sys/uio.h>

extern "C"
ssize_t syscall_SYS_writev(int fd, const struct iovec *iov, int iovcnt)
{
  if(fd <= 2)
  {
    ssize_t res = 0;
    for(int i = 0; i < iovcnt; i++)
    {
      auto* text = (const char*)iov[i].iov_base;
      auto len = iov[i].iov_len;
      kprintf("%.*s", len, text);
      res += len;
    }
    return res;
  }
  STRACE("syscall writev: fd=%d iov=%p iovcount=%d [base=%p sz=%d]\n",
    fd, iov, iovcnt, iov->iov_base, iov->iov_len);
  return 0;
}
