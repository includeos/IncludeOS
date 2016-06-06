//#define DEBUG
#include <fs/fat.hpp>

#include <cassert>
#include <fs/path.hpp>
#include <debug>

#include <cstring>
#include <memory>

#define likely(x)       __builtin_expect(!!(x), 1)
#define unlikely(x)     __builtin_expect(!!(x), 0)

inline size_t roundup(size_t n, size_t multiple)
{
  return ((n + multiple - 1) / multiple) * multiple;
}

namespace fs
{
  Buffer FAT::read(const Dirent& ent, uint64_t pos, uint64_t n)
  {
    // bounds check the read position and length
    uint32_t stapos = std::min(ent.size(), pos);
    uint32_t endpos = std::min(ent.size(), pos + n);
    // new length
    n = endpos - stapos;
    // cluster -> sector + position
    uint32_t sector = stapos / this->sector_size;
    uint32_t nsect = roundup(endpos, sector_size) / sector_size - sector;

    // the resulting buffer
    uint8_t* result = new uint8_t[n];

    // read @nsect sectors ahead
    buffer_t data = device.read_sync(this->cl_to_sector(ent.block) + sector, nsect);
    // where to start copying from the device result
    uint32_t internal_ofs = stapos % device.block_size();
    // when the offset is non-zero we aren't on a sector boundary
    if (internal_ofs != 0) {
      // so, we need to copy offset data to data buffer
      memcpy(result, data.get() + internal_ofs, n);
      data = buffer_t(result, std::default_delete<uint8_t[]>());
    }

    return Buffer(no_error, data, n);
  }

  error_t FAT::int_ls(uint32_t sector, dirvector& ents)
  {
    bool done = false;
    do {
      // read sector sync
      buffer_t data = device.read_sync(sector);
      if (unlikely(!data))
          return { error_t::E_IO, "Unable to read directory" };
      // parse directory into @ents
      done = int_dirent(sector, data.get(), ents);
      // go to next sector until done
      sector++;
    } while (!done);
    return no_error;
  }

  error_t FAT::traverse(Path path, dirvector& ents)
  {
    // start with root dir
    uint32_t cluster = 0;
    Dirent found(INVALID_ENTITY);

    while (!path.empty()) {

      uint32_t S = this->cl_to_sector(cluster);
      ents.clear(); // mui importante
      // sync read entire directory
      auto err = int_ls(S, ents);
      if (unlikely(err)) return err;
      // the name we are looking for
      std::string name = path.front();
      path.pop_front();

      // check for matches in dirents
      for (auto& e : ents)
      if (unlikely(e.name() == name)) {
        // go to this directory, unless its the last name
        debug("traverse_sync: Found match for %s", name.c_str());
        // enter the matching directory
        debug("\t\t cluster: %lu\n", e.block);
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
        debug("traverse_sync: NO MATCH for %s\n", name.c_str());
        return { error_t::E_NOENT, name };
      }
      // set next cluster
      cluster = found.block;
    }

    uint32_t S = this->cl_to_sector(cluster);
    // read result directory entries into ents
    return int_ls(S, ents);
  }

  FAT::List FAT::ls(const std::string& strpath)
  {
    auto ents = std::make_shared<dirvector> ();
    auto err = traverse(strpath, *ents);
    return { err, ents };
  }
  FAT::List FAT::ls(const Dirent& ent)
  {
    auto ents = std::make_shared<dirvector> ();
    // verify ent is a directory
    if (!ent.is_valid() || !ent.is_dir())
      return { { error_t::E_NOTDIR, ent.name() }, ents };
    // convert cluster to sector
    uint32_t S = this->cl_to_sector(ent.block);
    // read result directory entries into ents
    auto err = int_ls(S, *ents);
    return { err, ents };
  }

  FAT::Dirent FAT::stat(const std::string& strpath)
  {
    Path path(strpath);
    if (unlikely(path.empty())) {
      // root doesn't have any stat anyways (except ATTR_VOLUME_ID in FAT)
      return Dirent(INVALID_ENTITY);
    }

    debug("stat_sync: %s\n", path.back().c_str());
    // extract file we are looking for
    std::string filename = path.back();
    path.pop_back();

    // result directory entries are put into @dirents
    dirvector dirents;

    auto err = traverse(path, dirents);
    if (unlikely(err))
        return Dirent(INVALID_ENTITY); // for now

    // find the matching filename in directory
    for (auto& e : dirents)
    if (unlikely(e.name() == filename)) {
      // return this directory entry
      return e;
    }
    // entry not found
    return Dirent(INVALID_ENTITY);
  }
}
