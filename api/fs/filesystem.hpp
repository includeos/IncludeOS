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
#ifndef FS_FILESYSTEM_HPP
#define FS_FILESYSTEM_HPP

#include "common.hpp"

#include <string>
#include <vector>
#include <cstdint>
#include <functional>

namespace fs {

class FileSystem {
public:
  struct Dirent; //< Generic structure for directory entries
  
  using dirvector = std::vector<Dirent>;
  using dirvec_t  = std::shared_ptr<dirvector>;
  using buffer_t  = std::shared_ptr<uint8_t>;
  
  using on_mount_func = std::function<void(error_t)>;
  using on_ls_func    = std::function<void(error_t, dirvec_t)>;
  using on_read_func  = std::function<void(error_t, buffer_t, uint64_t)>;
  using on_stat_func  = std::function<void(error_t, const Dirent&)>;
  
  struct Buffer
  {
    error_t  err;
    buffer_t buffer;
    uint64_t len;
    
    Buffer(error_t e, buffer_t b, size_t l)
      : err(e), buffer(b), len(l) {}
  };
  
  enum Enttype {
    FILE,
    DIR,
    /** FAT puts disk labels in the root directory, hence: */
    VOLUME_ID,
    SYM_LINK,
    
    INVALID_ENTITY
  }; //< enum Enttype
  
  /** Generic structure for directory entries */
  struct Dirent {
    /** Default constructor */
    explicit Dirent(const Enttype t = INVALID_ENTITY, const std::string& n = "",
                    const uint64_t blk   = 0U, const uint64_t pr    = 0U,
                    const uint64_t sz    = 0U, const uint32_t attr  = 0U) :
      ftype     {t},
      fname     {n},
      block     {blk},
      parent    {pr},
      size      {sz},
      attrib    {attr},
      timestamp {0}
    {}
    
    Enttype     ftype;
    std::string fname;
    uint64_t    block;
    uint64_t    parent; //< Parent's block#
    uint64_t    size;
    uint32_t    attrib;
    int64_t     timestamp;
    
    Enttype type() const noexcept
    { return ftype; }
    
    // true if this dirent is valid
    // if not, it means don't read any values from the Dirent as they are not
    bool is_valid() const
    { return ftype != INVALID_ENTITY; }
    
    // most common types
    bool is_file() const noexcept
    { return ftype == FILE; }
    bool is_dir() const noexcept
    { return ftype == DIR; }
    
    // the entrys name
    const std::string& name() const noexcept
    { return fname; }
    
    // type converted to human-readable string
    std::string type_string() const {
      switch (ftype) {
      case FILE:
        return "File";
      case DIR:
        return "Directory";
      case VOLUME_ID:
        return "Volume ID";
        
      case INVALID_ENTITY:
        return "Invalid entity";
      default:
        return "Unknown type";
      } //< switch (type)
    }
  }; //< struct Dirent
  
   /** Mount this filesystem with LBA at @base_sector */
    virtual void mount(uint64_t lba, uint64_t size, on_mount_func on_mount) = 0;
    
    /** @param path: Path in the mounted filesystem */
    virtual void    ls(const std::string& path, on_ls_func) = 0;
    virtual error_t ls(const std::string& path, dirvec_t e) = 0;
    
    /** Read an entire file into a buffer, then call on_read */
    virtual void readFile(const std::string&, on_read_func) = 0;
    virtual void readFile(const Dirent& ent,  on_read_func) = 0;
    
    /** Read @n bytes from direntry from position @pos */
    virtual void   read(const Dirent&, uint64_t pos, uint64_t n, on_read_func) = 0;
    virtual Buffer read(const Dirent&, uint64_t pos, uint64_t n) = 0;
    
    /** Return information about a file or directory */
    virtual void   stat(const std::string& ent, on_stat_func) = 0;
    virtual Dirent stat(const std::string& ent) = 0;
    
    /** Returns the name of this filesystem */
    virtual std::string name() const = 0;

    /** Default destructor */
    virtual ~FileSystem() noexcept = default;
}; //< class FileSystem

// simplify initializing shared vector
inline FileSystem::dirvec_t new_shared_vector()
{
  return std::make_shared<FileSystem::dirvector> ();
}

} //< namespace fs

#endif //< FS_FILESYSTEM_HPP
