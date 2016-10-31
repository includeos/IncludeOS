#include <sys/stat.h>
#include <sys/socket.h>
#include <errno.h>

#ifndef DEFAULT_UMASK
#define DEFAULT_UMASK 002
#endif

int chmod(const char *path, mode_t mode) {
  errno = EROFS;
  return -1;
}

int mkdir(const char *path, mode_t mode)
{
  errno = EROFS;
  return -1;
}

int mkfifo(const char *path, mode_t mode)
{
  errno = EROFS;
  return -1;
}

int mknod(const char *path, mode_t mode, dev_t dev)
{
  errno = EROFS;
  return -1;
}

mode_t umask(mode_t cmask)
{
  return DEFAULT_UMASK;
}

int fchmod(int fildes, mode_t mode)
{
  errno = EROFS;
  return -1;
}
