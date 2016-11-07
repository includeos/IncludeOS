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
#include <delegate>

#ifndef likely
#define likely(x)       __builtin_expect(!!(x), 1)
#endif
#ifndef unlikely
#define unlikely(x)     __builtin_expect(!!(x), 0)
#endif

namespace fs {

  class File_system {
  public:
    struct Dirent; //< Generic structure for directory entries

    using dirvector = std::vector<Dirent>;
    using dirvec_t  = std::shared_ptr<dirvector>;
    using buffer_t  = std::shared_ptr<uint8_t>;

    using on_mount_func = delegate<void(error_t)>;
    using on_ls_func    = delegate<void(error_t, dirvec_t)>;
    using on_read_func  = delegate<void(error_t, buffer_t, uint64_t)>;
    using on_stat_func  = delegate<void(error_t, const Dirent&)>;

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
                      const uint64_t blk   = 0, const uint64_t pr    = 0,
                      const uint64_t sz    = 0, const uint32_t attr  = 0,
                      const uint32_t modt = 0)
      : ftype    {t},
        fname    {n},
        block    {blk},
        parent   {pr},
        size_    {sz},
        attrib   {attr},
        modif    {modt}
      {}

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

      // good luck
      uint64_t modified() const
      {
        /*
        uint32_t oldshit = modif;
        uint32_t day   = (oldshit & 0x1f);
        uint32_t month = (oldshit >> 5) & 0x0f;
        uint32_t year  = (oldshit >> 9) & 0x7f;
        oldshit >>= 16;
        uint32_t secs = (oldshit & 0x1f) * 2;
        uint32_t mins = (oldshit >> 5) & 0x3f;
        uint32_t hrs  = (oldshit >> 11) & 0x1f;
        // invalid timestamp?
        if (hrs > 23 or mins > 59 or secs > 59)
          return 0;
        */
        return modif;
      }

      uint64_t size() const noexcept {
        return size_;
      }

      Enttype     ftype;
      std::string fname;
      uint64_t    block;
      uint64_t    parent; //< Parent's block#
      uint64_t    size_;
      uint32_t    attrib;
      uint32_t    modif;
    }; //< struct Dirent

    struct List {
      error_t  error;
      dirvec_t entries;
    };

    /** Mount this filesystem with LBA at @base_sector */
    virtual void mount(uint64_t lba, uint64_t size, on_mount_func on_mount) = 0;

    /** @param path: Path in the mounted filesystem */
    virtual void  ls(const std::string& path, on_ls_func) = 0;
    virtual void  ls(const Dirent& entry,     on_ls_func) = 0;
    virtual List  ls(const std::string& path) = 0;
    virtual List  ls(const Dirent&) = 0;

    /** Read an entire file into a buffer, then call on_read */
    inline void read_file(const std::string& path, on_read_func on_read) {
      stat(path, [this, on_read, path](error_t err, const Dirent& ent) {
        if(unlikely(err))
          return on_read(err, nullptr, 0);

        if(unlikely(!ent.is_file()))
          return on_read({error_t::E_NOTFILE, path + " is not a file"}, nullptr, 0);

        read(ent, 0, ent.size(), on_read);
      });
    }

    inline Buffer read_file(const std::string& path) {
      auto ent = stat(path);

      if(unlikely(!ent.is_valid()))
        return {{error_t::E_NOENT, path + " not found"}, nullptr, 0};

      if(unlikely(!ent.is_file()))
        return {{error_t::E_NOTFILE, path + " is not a file"}, nullptr, 0};

      return read(ent, 0, ent.size());
    }

    /** Read @n bytes from direntry from position @pos */
    virtual void   read(const Dirent&, uint64_t pos, uint64_t n, on_read_func) = 0;
    virtual Buffer read(const Dirent&, uint64_t pos, uint64_t n) = 0;

    /** Return information about a file or directory */
    virtual void   stat(const std::string& ent, on_stat_func) = 0;
    virtual Dirent stat(const std::string& ent) = 0;

    /** Cached async stat */
    virtual void   cstat(const std::string&, on_stat_func) = 0;

    /** Returns the name of this filesystem */
    virtual std::string name() const = 0;

    /** Default destructor */
    virtual ~File_system() noexcept = default;
  }; //< class File_system

  // simplify initializing shared vector
  inline File_system::dirvec_t new_shared_vector()
  {
    return std::make_shared<File_system::dirvector> ();
  }

  using Dirent = File_system::Dirent;

} //< namespace fs

#endif //< FS_FILESYSTEM_HPP
