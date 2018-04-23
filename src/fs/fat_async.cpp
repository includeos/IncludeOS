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

#include <fs/fat.hpp>

#include <cassert>
#include <fs/path.hpp>
#include <cstring>
#include <memory>

//#define FS_PRINT(fmt, ...)  printf(fmt, ##__VA_ARGS__)
#define FS_PRINT(fmt, ...)  /** **/

inline size_t roundup(size_t n, size_t multiple)
{
  return ((n + multiple - 1) / multiple) * multiple;
}

namespace fs
{
  void FAT::int_ls(
     uint32_t   sector,
     Dirvec_ptr dirents,
     on_internal_ls_func callback) const
  {
    // list contents of meme sector by sector
    typedef delegate<void(uint32_t)> next_func_t;

    auto next = std::make_shared<next_func_t> ();
    auto weak_next = std::weak_ptr<next_func_t>(next);
    *next = next_func_t::make_packed(
    [this, callback, dirents, weak_next] (uint32_t sector)
    {
      FS_PRINT("int_ls: sec=%u\n", sector);
      auto next = weak_next.lock();
      device.read(
        sector,
        hw::Block_device::on_read_func::make_packed(
        [this, sector, callback, dirents, next] (buffer_t data)
        {
          if (data == nullptr) {
            // could not read sector
            callback({ error_t::E_IO, "Unable to read directory" }, dirents);
            return;
          }

          // parse entries in sector
          bool done = int_dirent(sector, data->data(), *dirents);
          if (done)
            // execute callback
            callback(no_error, dirents);
          else
            // go to next sector
            (*next)(sector+1);

        })
      ); // read root dir
    });

    // start reading sectors asynchronously
    (*next)(sector);
  }

  void FAT::traverse(std::shared_ptr<Path> path, cluster_func callback, const Dirent* const start) const
  {
    // parse this path into a stack of memes
    typedef delegate<void(uint32_t)> next_func_t;

    // asynch stack traversal
    auto next = std::make_shared<next_func_t> ();
    auto weak_next = std::weak_ptr<next_func_t>(next);
    *next = next_func_t::make_packed(
    [this, path, weak_next, callback] (uint32_t cluster)
    {
      if (path->empty()) {
        // attempt to read directory
        uint32_t S = this->cl_to_sector(cluster);

        // result allocated on heap
        auto dirents = std::make_shared<std::vector<Dirent>> ();

        int_ls(S, dirents,
        on_internal_ls_func::make_packed(
          [callback] (error_t error, Dirvec_ptr ents)
          { callback(error, ents);}
        ));

        return;
      }

      // retrieve next name
      std::string name = path->front();
      path->pop_front();

      uint32_t S = this->cl_to_sector(cluster);
      FS_PRINT("Current target: %s on cluster %u (sector %u)\n", name.c_str(), cluster, S);

      // result allocated on heap
      auto dirents = std::make_shared<std::vector<Dirent>> ();

      auto next = weak_next.lock();
      // list directory contents
      int_ls(
        S,
        dirents,
        on_internal_ls_func::make_packed(
        [name, dirents, next, callback] (error_t err, Dirvec_ptr ents)
        {
          if (UNLIKELY(err))
          {
            FS_PRINT("Could not find: %s\n", name.c_str());
            callback(err, dirents);
            return;
          }

          // look for name in directory
          for (auto& e : *ents)
          {
            if (UNLIKELY(e.name() == name))
            {
              // go to this directory, unless its the last name
              FS_PRINT("Found match for %s", name.c_str());
              // enter the matching directory
              FS_PRINT("\t\t cluster: %lu\n", e.block());
              // only follow directories
              if (e.type() == DIR)
                (*next)(e.block());
              else
                callback({ error_t::E_NOTDIR, e.name() }, dirents);
              return;
            }
          } // for (ents)

          FS_PRINT("NO MATCH for %s\n", name.c_str());
          callback({ error_t::E_NOENT, name }, dirents);
        })
      );

    });
    // start by reading provided dirent or root
    (*next)(start ? start->block() : 0);
  }

  void FAT::ls(const std::string& path, on_ls_func on_ls) const {

    // parse this path into a stack of names
    auto pstk = std::make_shared<Path> (path);

    traverse(pstk, on_ls);
  }
  void FAT::ls(const Dirent& ent, on_ls_func on_ls) const
  {
    auto dirents = std::make_shared<dirvector> ();
    // verify ent is a directory
    if (!ent.is_valid() || !ent.is_dir()) {
      on_ls( { error_t::E_NOTDIR, ent.name() }, dirents );
      return;
    }
    // convert cluster to sector
    uint32_t S = this->cl_to_sector(ent.block());
    // read result directory entries into ents
    int_ls(S, dirents, on_ls);
  }

  void FAT::read(const Dirent& ent, uint64_t pos, uint64_t n, on_read_func callback) const
  {
    // when n=0 roundup() will return an invalid value
    if (n == 0) {
      callback({ error_t::E_IO, "Zero read length" }, nullptr);
      return;
    }
    // bounds check the read position and length
    uint32_t stapos = std::min(ent.size(), pos);
    uint32_t endpos = std::min(ent.size(), pos + n);
    // new length
    n = endpos - stapos;
    // calculate start and length in sectors
    uint32_t sector = stapos / this->sector_size;
    uint32_t nsect = roundup(endpos, sector_size) / sector_size - sector;
    uint32_t internal_ofs = stapos % device.block_size();

    // cluster -> sector + position
    device.read(
      this->cl_to_sector(ent.block()) + sector,
      nsect,
      hw::Block_device::on_read_func::make_packed(
      [n, callback, internal_ofs] (buffer_t data)
      {
        if (!data) {
          // general I/O error occurred
          callback({ error_t::E_IO, "Unable to read file" }, nullptr);
          return;
        }

        // when the offset is non-zero we aren't on a sector boundary
        if (internal_ofs != 0) {
          // so, we need to create new buffer with offset data
          data = construct_buffer(data->begin() + internal_ofs, data->begin() + internal_ofs + n);
        }
        else {
          // when not offset all we have to do is resize the buffer down from
          // a sector size multiple to its given length
          data->resize(n);
        }

        callback(no_error, data);
      })
    );
  }

  void FAT::stat(Path_ptr path, on_stat_func func, const Dirent* const start) const
  {
    // manual lookup
    if (UNLIKELY(path->empty())) {
      // Note: could use ATTR_VOLUME_ID in FAT
      func(no_error, Dirent(this, DIR, "/", 0));
      return;
    }

    FS_PRINT("stat: %s\n", path->back().c_str());
    // extract file we are looking for
    std::string filename = path->back();
    path->pop_back();

    traverse(
      path,
      cluster_func::make_packed(
      [this, filename, func] (error_t error, Dirvec_ptr dirents)
      {
        if (UNLIKELY(error)) {
          // no path, no file!
          func(error, Dirent(this, INVALID_ENTITY, filename));
          return;
        }

        // find the matching filename in directory
        for (auto& e : *dirents) {
          if (UNLIKELY(e.name() == filename)) {
            // return this dir entry
            func(no_error, e);
            return;
          }
        }

        // not found
        func({ error_t::E_NOENT, filename }, Dirent(this, INVALID_ENTITY, filename));
      }),
      start
    );
  }

  void FAT::cstat(const std::string& strpath, on_stat_func func)
  {
    // cache lookup
    auto it = stat_cache.find(strpath);
    if (it != stat_cache.end()) {
      FS_PRINT("used cached stat for %s\n", strpath.c_str());
      func(no_error, it->second);
      return;
    }

    File_system::stat(
      strpath,
      fs::on_stat_func::make_packed(
      [this, strpath, func] (error_t error, const Dirent& ent)
      {
        stat_cache.emplace(strpath, ent);
        func(error, ent);
      })
    );
  }
}
