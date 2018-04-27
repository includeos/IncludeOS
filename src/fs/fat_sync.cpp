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

#include <fs/fat.hpp>

#include <fs/path.hpp>
#include <cassert>
#include <cstring>
#include <memory>
#include <common>

inline size_t roundup(size_t n, size_t multiple)
{
  return ((n + multiple - 1) / multiple) * multiple;
}

//#define FS_PRINT(fmt, ...)  printf(fmt, ##__VA_ARGS__)
#define FS_PRINT(fmt, ...)  /** **/

namespace fs
{
  Buffer FAT::read(const Dirent& ent, uint64_t pos, uint64_t n) const
  {
    // bounds check the read position and length
    auto stapos = std::min(ent.size(), pos);
    auto endpos = std::min(ent.size(), pos + n);
    // new length
    n = endpos - stapos;
    // cluster -> sector + position
    auto sector = stapos / this->sector_size;
    auto nsect = roundup(endpos, sector_size) / sector_size - sector;

    // read @nsect sectors ahead
    buffer_t data = device.read_sync(this->cl_to_sector(ent.block()) + sector, nsect);
    // where to start copying from the device result
    auto internal_ofs = stapos % device.block_size();
    // when the offset is non-zero we aren't on a sector boundary
    if (internal_ofs != 0) {
      data = construct_buffer(data->begin() + internal_ofs, data->begin() + internal_ofs + n);
    }
    else {
      // when not offset all we have to do is resize the buffer down from
      // a sector size multiple to its given length
      data->resize(n);
    }
    return Buffer(no_error, std::move(data));
  }

  error_t FAT::int_ls(uint32_t sector, dirvector& ents) const
  {
    bool done = false;
    do {
      // read sector sync
      buffer_t data = device.read_sync(sector);
      if (UNLIKELY(!data))
          return { error_t::E_IO, "Unable to read directory" };
      // parse directory into @ents
      done = int_dirent(sector, data->data(), ents);
      // go to next sector until done
      sector++;
    } while (!done);
    return no_error;
  }

  error_t FAT::traverse(Path path, dirvector& ents, const Dirent* const start) const
  {
    // start with given entry (defaults to root)
    uint32_t cluster = start ? start->block() : 0;
    Dirent found(this, INVALID_ENTITY);

    while (!path.empty()) {

      auto S = this->cl_to_sector(cluster);
      ents.clear(); // mui importante
      // sync read entire directory
      auto err = int_ls(S, ents);
      if (UNLIKELY(err)) return err;
      // the name we are looking for
      const std::string name = path.front();
      path.pop_front();

      // check for matches in dirents
      for (auto& e : ents)
      if (UNLIKELY(e.name() == name)) {
        // go to this directory, unless its the last name
        FS_PRINT("traverse_sync: Found match for %s", name.c_str());
        // enter the matching directory
        FS_PRINT("\t\t cluster: %lu\n", e.block());
        // only follow if the name is a directory
        if (e.type() == DIR) {
          found = e;
          break;
        }
        else {
          // not dir = error, for now
          return { error_t::E_NOTDIR, "Cannot list non-directory" };
        }
      } // for (ents)

      // validate result
      if (found.type() == INVALID_ENTITY) {
        FS_PRINT("traverse_sync: NO MATCH for %s\n", name.c_str());
        return { error_t::E_NOENT, name };
      }
      // set next cluster
      cluster = found.block();
    }

    auto S = this->cl_to_sector(cluster);
    // read result directory entries into ents
    ents.clear(); // mui importante!
    return int_ls(S, ents);
  }

  List FAT::ls(const std::string& strpath) const
  {
    auto ents = std::make_shared<dirvector> ();
    auto err = traverse(strpath, *ents);
    return { err, ents };
  }

  List FAT::ls(const Dirent& ent) const
  {
    auto ents = std::make_shared<dirvector> ();
    // verify ent is a directory
    if (!ent.is_valid() || !ent.is_dir())
      return { { error_t::E_NOTDIR, ent.name() }, ents };
    // convert cluster to sector
    auto S = this->cl_to_sector(ent.block());
    // read result directory entries into ents
    auto err = int_ls(S, *ents);
    return { err, ents };
  }

  Dirent FAT::stat(Path path, const Dirent* const start) const
  {
    if (UNLIKELY(path.empty())) {
      return Dirent(this, Enttype::DIR, "/", 0);
    }

    FS_PRINT("stat_sync: %s\n", path.back().c_str());
    // extract file we are looking for
    const std::string filename = path.back();
    path.pop_back();

    // result directory entries are put into @dirents
    dirvector dirents;

    auto err = traverse(path, dirents, start);
    if (UNLIKELY(err))
      return Dirent(this, INVALID_ENTITY); // for now

    // find the matching filename in directory
    for (auto& e : dirents)
    if (UNLIKELY(e.name() == filename)) {
      // return this directory entry
      return e;
    }
    // entry not found
    return Dirent(this, INVALID_ENTITY);
  }
}
