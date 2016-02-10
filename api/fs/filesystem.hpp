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
#ifndef FS_FILESYS_HPP
#define FS_FILESYS_HPP

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace fs
{
  typedef bool error_t;
  
  class FileSystem
  {
  public:
    enum Enttype
    {
      FILE,
      DIR,
      // FAT puts disk labels in the root directory, hence:
      VOLUME_ID,
    };
    
    // generic structure for directory entries
    struct Dirent
    {
      Dirent() {}
      Dirent(std::string n, Enttype t, uint64_t blk, uint64_t pr, uint64_t sz, uint32_t attr)
        : name(n), type(t), block(blk), parent(pr), size(sz), attrib(attr), timestamp(0) {}
      
      std::string name;
      Enttype  type;
      uint64_t block;
      uint64_t parent; // parent's block#
      uint64_t size;
      uint32_t attrib;
      int64_t  timestamp;
      
      std::string type_string() const
      {
        switch (type)
        {
        case FILE:
          return "File";
        case DIR:
          return "Directory";
        case VOLUME_ID:
          return "Volume ID";
        default:
          return "Unknown type";
        }
      }
      
    };
    
    typedef std::shared_ptr<std::vector<Dirent>> dirvec_t;
    
    typedef std::function<void(error_t)> on_mount_func;
    typedef std::function<void(error_t, dirvec_t)> on_ls_func;
    typedef std::function<void(error_t, uint8_t*, size_t)> on_read_func;
    typedef std::function<void(error_t, const Dirent&)> on_stat_func;
    
    
    // 0   = Mount MBR
    // 1-4 = Mount VBR 1-4
    virtual void mount(uint8_t partid, on_mount_func on_mount) = 0;
    
    // path is a path in the mounted filesystem
    virtual void ls(const std::string& path, on_ls_func) = 0;
    
    // read an entire file into a buffer, then call on_read
    virtual void readFile(const std::string&, on_read_func) = 0;
    virtual void readFile(const Dirent& ent, on_read_func) = 0;
    
    // return information about a file or directory
    virtual void stat(const std::string& ent, on_stat_func) = 0;
    
    // returns the name of this filesystem
    virtual std::string name() const = 0;
  };
}

#endif
