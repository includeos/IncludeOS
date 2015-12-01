// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015 Oslo and Akershus University College of Applied Sciences
// and Alfred Bratterud
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
//     http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

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
