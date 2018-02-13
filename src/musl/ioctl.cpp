#include <sys/ioctl.h>
#include <fd_map.hpp>

#include "stub.hpp"

static int sys_ioctl(int fd, int req, void* arg) {
  if (fd == 1 or fd == 2) {
    if (req == TIOCGWINSZ) {
      winsize* ws = (winsize*)arg;
      ws->ws_row = 25;
      ws->ws_col = 80;
      return 0;
    }

    if (req == TIOCSWINSZ) {
      winsize* ws = (winsize*)arg;
    }

    return 0;
  }

  try {
    auto& fildes = FD_map::_get(fd);
    return fildes.ioctl(req, arg);
  }
  catch(const FD_not_found&) {
    errno = EBADF;
    return -1;
  }
}

extern "C"
int syscall_SYS_ioctl(int fd, int req, void* arg) {
  return stubtrace(sys_ioctl, "ioctl", fd, req, arg);
}
