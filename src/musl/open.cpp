#include "common.hpp"
#include <sys/types.h>
#include <fs/vfs.hpp>

static long sys_open(const char *pathname, int /*flags*/, mode_t /*mode = 0*/)
{
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

  return -ENOENT;
}

extern "C"
long syscall_SYS_open(const char *pathname, int flags, mode_t mode = 0) {
  return strace(sys_open, "open", pathname, flags, mode);
}
