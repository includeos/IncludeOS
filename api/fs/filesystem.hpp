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

#ifndef FS_FILESYSTEM_HPP
#define FS_FILESYSTEM_HPP

#include "common.hpp"
#include <string>
#include <cstdint>

namespace fs {

  struct Dirent;

  struct File_system {
    /** Get unique id for underlying device */
    virtual int device_id() const noexcept = 0;

    /** Print subtree from given path */
    error_t print_subtree(const std::string& path);

    /** @param path: Path in the initialized filesystem */
    virtual void  ls(const std::string& path, on_ls_func) const = 0;
    virtual void  ls(const Dirent& entry,     on_ls_func) const = 0;
    virtual List  ls(const std::string& path) const = 0;
    virtual List  ls(const Dirent&) const = 0;

    /** Read an entire file into a buffer, async, then call on_read */
    void read_file(const std::string& path, on_read_func on_read) const;

    /** Read an entire file sync **/
    Buffer read_file(const std::string& path) const;

    /** Read @n bytes from direntry from position @pos  - async */
    virtual void   read(const Dirent&, uint64_t pos, uint64_t n, on_read_func) const = 0;

    /** Read - sync */
    virtual Buffer read(const Dirent&, uint64_t pos, uint64_t n) const = 0;

    /** Return information about a file or directory - async */
    virtual void stat(Path_ptr, on_stat_func fn, const Dirent* const = nullptr) const = 0;

    /** Stat async - for various types of path initializations **/
    template <typename P = Path>
    inline void stat(P pathstr, on_stat_func fn, const Dirent* const = nullptr) const;

    /** Return information about a file or directory relative to dirent - sync*/
    virtual Dirent stat(Path, const Dirent* const = nullptr) const = 0;

    /** Return information about a file or directory relative to dirent - sync*/
    template <typename P = Path>
    inline Dirent stat(P, const Dirent* const = nullptr) const;

    /** Cached async stat */
    virtual void cstat(const std::string& pathstr, on_stat_func) = 0;

    /** Returns the name of this filesystem */
    virtual std::string name() const = 0;

    /** Returns the block size of this filesystem */
    virtual uint64_t block_size() const = 0;

    /** Initialize this filesystem with LBA at @base_sector */
    virtual void init(uint64_t lba, uint64_t size, on_init_func on_init) = 0;

    /** Default destructor */
    virtual ~File_system() noexcept = default;
  }; //< class File_system

} //< namespace fs

  /** Inline implementations **/

#include <fs/dirent.hpp>

namespace fs {

  template <typename P>
  void File_system::stat(P path, on_stat_func fn, const Dirent* const dir) const {
    auto path_ptr = std::make_shared<Path> (path);
    stat(path_ptr, fn, dir);
  }

  template <typename P>
  Dirent File_system::stat(P pathstr, const Dirent* const dir) const {
    return stat(Path{pathstr}, dir);
  }

} //< namespace fs

#endif //< FS_FILESYSTEM_HPP
