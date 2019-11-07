
#include <posix/fd.hpp>
#include <fcntl.h>
#include <cstdarg>
#include <errno.h>

int FD::fcntl(int cmd, va_list list)
{
  //PRINT("fcntl(%d)\n", cmd);
  switch (cmd) {
  case F_GETFD:
      // return descriptor flags
      return dflags;
  case F_SETFD:
      // set desc flags from va_list
      dflags = va_arg(list, int);
      return 0;
  case F_GETFL:
      // return file access flags
      return fflags;
  case F_SETFL:
      // set file access flags
      fflags = va_arg(list, int);
      return 0;
  case F_DUPFD:
  case F_DUPFD_CLOEXEC:
  default:
      errno = EINVAL;
      return -1;
  }
}

int FD::ioctl(int /*req*/, void* /*arg*/)
{
  //PRINT("ioctl(%d, %p) = -1\n", req, arg);
  errno = ENOSYS;
  return -1;
}

int FD::getsockopt(int /*fd*/, int, void *__restrict__, socklen_t *__restrict__)
{
  //PRINT("getsockopt(%d) = -1\n", fd);
  errno = ENOTSOCK;
  return -1;
}
int FD::setsockopt(int /*fd*/, int, const void *, socklen_t)
{
  //PRINT("setsockopt(%d) = -1\n", fd);
  errno = ENOTSOCK;
  return -1;
}
