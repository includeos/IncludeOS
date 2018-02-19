#include "stub.hpp"
#include <sys/stat.h>

#include <fs/vfs.hpp>

int sys_open(const char *pathname, int flags, mode_t mode = 0) {
  try {
    auto& entry = fs::VFS::get<FD_compatible>(pathname);
    auto& fd = entry.open_fd();
    return fd.get_id();
  }
  catch(const fs::VFS_err&) {
    // VFS error for one of many reasons (not mounted, not fd compatible etc)
  }
  catch(const std::bad_function_call&) {
    // Open fd delegate not set
  }

  errno = ENOENT;
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
