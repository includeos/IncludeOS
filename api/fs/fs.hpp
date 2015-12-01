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

#pragma once

#include <stdint.h>
#include <string>
#include "common.hpp"

namespace fs
{
  #pragma interface   // __declspec(novtable)
  class FilesystemBase
  {
  public:
    virtual inode_t
    open(const std::string& name, int flags, ...) = 0;
    virtual int
    read(inode_t file, const std::string& ptr, int len) = 0;
    virtual int
    write(inode_t file, const std::string& ptr, int len) = 0;
    virtual int
    close(inode_t file) = 0;
    
    virtual inode_t
    symlink(const std::string& from, const std::string& to) = 0;
    virtual int
    stat(const std::string& path, void* buf) = 0;
    
    virtual int
    readdir(const std::string& path, void* ptr) = 0;
    virtual int
    rmdir(const std::string& path) = 0;
    virtual int
    mkdir(const std::string& path) = 0;
    
  };
  
}
