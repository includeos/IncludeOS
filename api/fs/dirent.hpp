// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015-2016 Oslo and Akershus University College of Applied Sciences
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
#ifndef FS_DIRENT_HPP
#define FS_DIRENT_HPP

#include "common.hpp"

namespace fs {

  struct File_system;

  /** Generic structure for directory entries */
  struct Dirent {

    /** Constructor */
    explicit Dirent(observer_ptr<File_system> fs, const Enttype t = INVALID_ENTITY, const std::string& n = "",
                    const uint64_t blk   = 0, const uint64_t pr    = 0,
                    const uint64_t sz    = 0, const uint32_t attr  = 0,
                    const uint32_t modt = 0)
      : fs_ {fs}, ftype {t}, fname_ {n},
        block_ {blk}, parent_ {pr},
        size_{sz}, attrib_ {attr},
        modif {modt}
    {}

    Enttype type() const noexcept
    { return ftype; }

    const std::string& name() const noexcept
    { return fname_; }

    uint64_t block() const noexcept
    { return block_; }

    uint64_t size() const noexcept
    { return size_; }

    uint32_t attrib() const noexcept
    { return attrib_; }

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


    // true if this dirent is valid
    // if not, it means don't read any values from the Dirent as they are not
    bool is_valid() const
    { return ftype != INVALID_ENTITY; }

    // most common types
    bool is_file() const noexcept
    { return ftype == FILE; }

    bool is_dir() const noexcept
    { return ftype == DIR; }

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

    /** Read async **/
    inline void read(uint64_t pos, uint64_t n, on_read_func fn);

    /** Read sync **/
    inline Buffer read(uint64_t pos, uint64_t n);

    /** Read the whole file, sync, to string **/
    inline std::string read();

    /** List contents async **/
    inline void ls(on_ls_func fn);

    /** List contents sync **/
    inline List ls();

    /** Get a dirent by path, relative to here **/
    template <typename P = std::initializer_list<std::string> >
    inline Dirent stat(P path);

  private:
    observer_ptr<File_system> fs_;
    Enttype     ftype;
    std::string fname_;
    uint64_t    block_;
    uint64_t    parent_; //< Parent's block#
    uint64_t    size_;
    uint32_t    attrib_;
    uint32_t    modif;
  }; //< struct Dirent

} //< namespace fs

  /** Inline Implementations **/

#include <fs/filesystem.hpp>

namespace fs {
  void Dirent::read(uint64_t pos, uint64_t n, on_read_func fn) {
    fs_->read(*this, pos, n, fn);
  }

  /** Read sync **/
  Buffer Dirent::read(uint64_t pos, uint64_t n) {
    return fs_->read(*this, pos, n);
  }

  /** Read the whole file, sync, to string **/
  std::string Dirent::read() {
    return read(0, size_).to_string();
  }

  /** List contents async **/
  void Dirent::ls(on_ls_func fn) {
    fs_->ls(*this, fn);
  }

  /** List contents sync **/
  List Dirent::ls() {
    return fs_->ls(*this);
  }

  template <typename P>
  Dirent Dirent::stat(P path) {
    return fs_->stat(Path{path}, this);
  };

} //< namespace fs


#endif //< FS_DIRENT_HPP
