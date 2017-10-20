#include <sys/ioctl.h>
#include <cstdio>

int ioctl(int fd, unsigned long request, ...)
{
  printf("ioctl(%d, %x)\n", fd, request);
}
