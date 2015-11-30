#include <fs/common.hpp>

std::string fs_error_string(int error)
{
  switch (error)
  {
  case 0:
    return "No error";
  case -ENOENT:
    return "No such file or directory";
  case -EIO:
    return "I/O Error";
  case -EEXIST:
    return "File already exists";
  case -ENOTDIR:
    return "Not a directory";
  case -EINVAL:
    return "Invalid argument";
  case -ENOSPC:
    return "No space left on device";
  case -ENOTEMPTY:
    return "Directory not empty";
  
  }
  return "Invalid error code";
}
