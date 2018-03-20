#include "common.hpp"
#include <unistd.h>

#include <fs/vfs.hpp>

static std::string cwd{"/"};

static long sys_chdir(const char* path)
{
  // todo: handle relative path
  // todo: handle ..
  if (UNLIKELY(path == nullptr))
    return -ENOENT;

  if (UNLIKELY(strlen(path) < 1))
    return -ENOENT;

  if (strcmp(path, ".") == 0)
    return 0;

  std::string desired_path;
  if (*path != '/')
  {
    desired_path = cwd;
    if (!(desired_path.back() == '/')) desired_path += "/";
    desired_path += path;
  }
  else
  {
    desired_path.assign(path);
  }
  try {
    auto ent = fs::VFS::stat_sync(desired_path);
    if (ent.is_dir())
    {
      cwd = desired_path;
      assert(cwd.front() == '/');
      assert(cwd.find("..") == std::string::npos);
      return 0;
    }
    else
    {
      // path is not a dir
      return -ENOTDIR;
    }
  }
  catch (const fs::Err_not_found& e) {
    return -ENOTDIR;
  }
}

long sys_getcwd(char *buf, size_t size)
{
  Expects(cwd.front() == '/'); // dangerous?

  if (UNLIKELY(buf == nullptr or size == 0))
    return -EINVAL;

  if ((cwd.length() + 1) < size)
  {
    snprintf(buf, cwd.length()+1, "%s", cwd.c_str());
    return cwd.length()+1;
  }
  else
  {
    return -ERANGE;
  }
}

extern "C"
long syscall_SYS_chdir(const char* path) {
  return strace(sys_chdir, "chdir", path);
  //return sys_chdir(path);
}

extern "C"
long syscall_SYS_getcwd(char *buf, size_t size) {
  return strace(sys_getcwd, "getcwd", buf, size);
}
