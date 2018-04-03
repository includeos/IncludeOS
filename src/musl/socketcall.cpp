#include <errno.h>

extern "C"
long socketcall_socket()
{
  return -ENOSYS;
}
extern "C"
long socketcall_connect()
{
  return -ENOSYS;
}
extern "C"
long socketcall_shutdown()
{
  return -ENOSYS;
}
