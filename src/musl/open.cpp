#include "common.hpp"
#include <sys/stat.h>

extern "C" {
int syscall_SYS_open(const char *pathname, int flags, mode_t mode = 0) {
  STRACE("open pathname=%s, flags=0x%x, mode=0x%x",
         pathname, flags, mode);
}

int syscall_SYS_creat(const char *pathname, mode_t mode) {
  STUB("creat");
}
} // extern "C"
