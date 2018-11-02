#include "common.hpp"
#include <sys/types.h>
#include <fs/vfs.hpp>
#include <posix/fd_map.hpp>
#include <posix/file_fd.hpp>

static long sys_open(const char *pathname, int /*flags*/, mode_t /*mode = 0*/) {
  if (UNLIKELY(pathname == nullptr))
    return -EFAULT;

  if (UNLIKELY(pathname[0] == 0))
    return -ENOENT;

  try {
    auto& entry = fs::VFS::get<FD_compatible>(pathname);
    auto& fd = entry.open_fd();
    return fd.get_id();
  }
  // Not FD_compatible, try dirent
  catch(const fs::VFS_err& err)
  {
    try {
      auto ent = fs::VFS::stat_sync(pathname);
      if (ent.is_valid())
      {
        auto& fd = FD_map::_open<File_FD>(ent);
        return fd.get_id();
      }
      return -ENOENT;
    }
    catch(...)
    {
      return -ENOENT;
    }
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
