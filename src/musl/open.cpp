#include "stub.hpp"
#include <sys/stat.h>

int sys_open(const char *pathname, int flags, mode_t mode = 0) {
  return -1;
}

int sys_creat(const char *pathname, mode_t mode) {
  return -1;
}

extern "C" {
int syscall_SYS_open(const char *pathname, int flags, mode_t mode = 0) {
  return stubtrace(sys_open, "open", pathname, flags, mode);
}

int syscall_SYS_creat(const char *pathname, mode_t mode) {
  return stubtrace(sys_creat, "creat", pathname, mode);
}

} // extern "C"
