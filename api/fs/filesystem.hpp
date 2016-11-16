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

#ifndef likely
#define likely(x)       __builtin_expect(!!(x), 1)
#endif
#ifndef unlikely
#define unlikely(x)     __builtin_expect(!!(x), 0)
#endif

namespace fs {

  struct Dirent;

  struct File_system {

    /** Mount this filesystem with LBA at @base_sector */
    virtual void mount(uint64_t lba, uint64_t size, on_mount_func on_mount) = 0;

    /** @param path: Path in the mounted filesystem */
    virtual void  ls(const std::string& path, on_ls_func) = 0;
    virtual void  ls(const Dirent& entry,     on_ls_func) = 0;
    virtual List  ls(const std::string& path) = 0;
    virtual List  ls(const Dirent&) = 0;

    /** Read an entire file into a buffer, async, then call on_read */
    void read_file(const std::string& path, on_read_func on_read);

    /** Read an entire file sync **/
    Buffer read_file(const std::string& path);

    /** Read @n bytes from direntry from position @pos  - async */
    virtual void   read(const Dirent&, uint64_t pos, uint64_t n, on_read_func) = 0;

    /** Read - sync */
    virtual Buffer read(const Dirent&, uint64_t pos, uint64_t n) = 0;

    /** Return information about a file or directory - async */
    virtual void stat(Path_ptr, on_stat_func fn) = 0;

    /** Return information about a file or directory - sync */
    virtual Dirent stat(Path, const handle<const Dirent> = nullptr) = 0;

    /** Stat async - string variant **/
    inline void stat(const std::string& pathstr, on_stat_func fn);

    /** Stat sync - string variant **/
    inline Dirent stat(const std::string& pathstr);

    /** Cached async stat */
    virtual void cstat(const std::string& pathstr, on_stat_func) = 0;

    /** Returns the name of this filesystem */
    virtual std::string name() const = 0;

    /** Default destructor */
    virtual ~File_system() noexcept = default;
  }; //< class File_system

  // simplify initializing shared vector
  inline dirvec_t new_shared_vector()
  {
    return std::make_shared<dirvector> ();
  }


} //< namespace fs

  /** Inline implementations **/

#include <fs/dirent.hpp>

namespace fs {

  void File_system::stat(const std::string& pathstr, on_stat_func fn) {
    auto path = std::make_shared<Path> (pathstr);
    stat(path, fn);
  }

  Dirent File_system::stat(const std::string& pathstr) {
    return stat(Path{pathstr});
  }


} //< namespace fs

#endif //< FS_FILESYSTEM_HPP
