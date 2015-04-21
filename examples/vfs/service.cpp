#include <os>
#include <class_dev.hpp>
#include <fs/vfs>

#include <assert.h>
#include <iostream>

int verbose_mkdir(fs::VFS& fs, const std::string& path)
{
  int res = fs.mkdir(path);
  std::cout << "mkdir(" << path << "): " << res << " [" << fs_error_string(res) << "]" << std::endl;
  return res;
}
int verbose_rmdir(fs::VFS& fs, const std::string& path)
{
  int res = fs.rmdir(path);
  std::cout << "rmdir(" << path << "): " << res << " [" << fs_error_string(res) << "]" << std::endl;
  return res;
}

void Service::start()
{
  std::cout << "*** Service is up - with OS Included! ***" << std::endl;
  
  fs::VFS filesystem(64, 128);
  
  assert(0 ==  // 2
    verbose_mkdir(filesystem, "/test"));
  assert(0 ==  // 3
    verbose_mkdir(filesystem, "/test/test"));
  
  std::cout << std::endl;
  
  assert(-ENOTEMPTY == // 2
    verbose_rmdir(filesystem, "/test"));
  
  std::cout << std::endl;
  
  assert(0 ==  // 3
    verbose_rmdir(filesystem, "/test/test"));
  
  //assert(0 ==
  //  verbose_rmdir(filesystem, "/test"));
  
  //assert(-ENOENT == 
  //  verbose_mkdir(filesystem, "/test/test/test"));
  
  std::cout << "Service out!" << std::endl;
}
