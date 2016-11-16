#include <sys/select.h>
#include <errno.h>

int  pselect(int, fd_set *__restrict__, fd_set *__restrict__, fd_set *__restrict__,
         const struct timespec *__restrict__, const sigset_t *__restrict__)
{
  errno = EINVAL;
  return -1;
}
int  select(int num, 
            fd_set *__restrict__ reads, 
            fd_set *__restrict__ writes, 
            fd_set *__restrict__ excepts,
            struct timeval *__restrict__)
{
  // monitor file descriptors given in read, write and exceptional fd sets
  static bool event_read = false;
  static bool event_writ = false;
  static bool event_exce = false;
  
  /*
  for (each fd in fd_read) {
    fd->monitor_read()
  }
  */
}
