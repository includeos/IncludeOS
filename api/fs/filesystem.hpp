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

#include <memory>
#include <string>
#include <vector>
#include <cstdint>
#include <functional>

#include "error.hpp"

namespace fs {

class FileSystem {
public:
  struct Dirent; //< Generic structure for directory entries
  
  using dirvec_t = std::shared_ptr<std::vector<Dirent>>;
  
  using on_mount_func = std::function<void(error_t)>;
  using on_ls_func    = std::function<void(error_t, dirvec_t)>;
  using on_read_func  = std::function<void(error_t, uint8_t*, size_t)>;
  using on_stat_func  = std::function<void(error_t, const Dirent&)>;

  enum Enttype {
    FILE,
    DIR,
    /** FAT puts disk labels in the root directory, hence: */
    VOLUME_ID,
  }; //< enum Enttype
  
  /** Generic structure for directory entries */
  struct Dirent {
    /** Default constructor */
    explicit Dirent(const std::string& n = "", const Enttype  t    = FILE,
                    const uint64_t blk   = 0U, const uint64_t pr   = 0U,
                    const uint64_t sz    = 0U, const uint32_t attr = 0U) :
      name      {n},
      type      {t},
      block     {blk},
      parent    {pr},
      size      {sz},
      attrib    {attr},
      timestamp {0}
    {}
    
    std::string name;
    Enttype     type;
    uint64_t    block;
    uint64_t    parent; //< Parent's block#
    uint64_t    size;
    uint32_t    attrib;
    int64_t     timestamp;
    
    std::string type_string() const {
      switch (type) {
      case FILE:
        return "File";
      case DIR:
        return "Directory";
      case VOLUME_ID:
        return "Volume ID";
      default:
        return "Unknown type";
      } //< switch (type)
    }
  }; //< struct Dirent
  
   /** Mount this filesystem with LBA at @base_sector */
    virtual void mount(uint64_t lba, uint64_t size, on_mount_func on_mount) = 0;
    
    /** @param path: Path in the mounted filesystem */
    virtual void ls(const std::string& path, on_ls_func) = 0;
    
    /** Read an entire file into a buffer, then call on_read */
    virtual void readFile(const std::string&, on_read_func) = 0;
    virtual void readFile(const Dirent& ent,  on_read_func) = 0;
    
    /** Return information about a file or directory */
    virtual void stat(const std::string& ent, on_stat_func) = 0;
    
    /** Returns the name of this filesystem */
    virtual std::string name() const = 0;

    /** Default destructor */
    virtual ~FileSystem() noexcept = default;
}; //< class FileSystem
} //< namespace fs

#endif //< FS_FILESYS_HPP
