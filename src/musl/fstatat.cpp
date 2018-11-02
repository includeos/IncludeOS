#include "common.hpp"
#include <sys/stat.h>

#include <posix/fd_map.hpp>

long sys_getcwd(char *buf, size_t size);
long sys_stat(const char *path, struct stat *buf);

static long sys_fstatat(int fd, const char *path, struct stat *buf, int flag)
{
  if (fd == AT_FDCWD)
  {
    char cwd_buf[PATH_MAX-1];
    char abs_path[PATH_MAX];
    if (sys_getcwd(cwd_buf, PATH_MAX-1) > 0) {
      snprintf(abs_path, PATH_MAX, "%s/%s", cwd_buf, path);
    }
    return sys_stat(abs_path, buf);
  }
  else
  {
   if(auto* fildes = FD_map::_get(fd); fildes)
      return fildes->fstatat(path, buf, flag);

    return -EBADF;
  }
}

extern "C"
long syscall_SYS_fstatat(int fd, const char *path, struct stat *stat_buf, int flag) {
  return strace(sys_fstatat, "fstatat", fd, path, stat_buf, flag);
}
