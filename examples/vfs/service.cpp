#include <os>
#include <class_dev.hpp>
#include <fs/vfs>

#include <assert.h>

#include <iostream>

void Service::start()
{
  std::cout << "*** Service is up - with OS Included! ***" << std::endl;
  
  fs::VFS filesystem(64, 128);
  
  
  std::cout << "Service out!" << std::endl;
}
