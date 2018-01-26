#include "common.hpp"
#include <sys/ioctl.h>

#warning stub

extern "C"
int syscall_SYS_ioctl(int fd, int req, void* arg) {
  STRACE("syscall ioctl: fd=%i, req=0x%x\n",
         fd, req);

  if (req == TIOCGWINSZ) {
    kprintf("\t* Wants to get winsize\n");
    winsize* ws = (winsize*)arg;
    ws->ws_row = 25;
    ws->ws_col = 80;
    return 0;
  }

  if (req == TIOCSWINSZ) {
    winsize* ws = (winsize*)arg;
    kprintf("\t* Wants to set winsize to %i x %i\n",
            ws->ws_row, ws->ws_col);
  }

  return 0;
}
