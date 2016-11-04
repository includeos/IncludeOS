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
  void FAT::int_ls(
     uint32_t sector,
     dirvec_t dirents,
     on_internal_ls_func callback)
  {
    // list contents of meme sector by sector
    typedef std::function<void(uint32_t)> next_func_t;

    auto next = std::make_shared<next_func_t> ();
    auto weak_next = std::weak_ptr<next_func_t>(next);
    *next =
    [this, sector, callback, dirents, weak_next] (uint32_t sector)
    {
      debug("int_ls: sec=%u\n", sector);
      auto next = weak_next.lock();
      device.read(sector,
      [this, sector, callback, dirents, next] (buffer_t data) {

        if (!data) {
          // could not read sector
          callback({ error_t::E_IO, "Unable to read directory" }, dirents);
          return;
        }

        // parse entries in sector
        bool done = int_dirent(sector, data.get(), *dirents);
        if (done)
          // execute callback
          callback(no_error, dirents);
        else
          // go to next sector
          (*next)(sector+1);

      }); // read root dir
    };

    // start reading sectors asynchronously
    (*next)(sector);
  }

  void FAT::traverse(std::shared_ptr<Path> path, cluster_func callback)
  {
    // parse this path into a stack of memes
    typedef std::function<void(uint32_t)> next_func_t;

    // asynch stack traversal
    auto next = std::make_shared<next_func_t> ();
    auto weak_next = std::weak_ptr<next_func_t>(next);
    *next =
    [this, path, weak_next, callback] (uint32_t cluster) {

      if (path->empty()) {
        // attempt to read directory
        uint32_t S = this->cl_to_sector(cluster);

        // result allocated on heap
        auto dirents = std::make_shared<std::vector<Dirent>> ();

        int_ls(S, dirents,
        [callback] (error_t error, dirvec_t ents) {
          callback(error, ents);
        });
        return;
      }

      // retrieve next name
      std::string name = path->front();
      path->pop_front();

      uint32_t S = this->cl_to_sector(cluster);
      debug("Current target: %s on cluster %u (sector %u)\n", name.c_str(), cluster, S);

      // result allocated on heap
      auto dirents = std::make_shared<std::vector<Dirent>> ();

      auto next = weak_next.lock();
      // list directory contents
      int_ls(S, dirents,
      [name, dirents, next, callback] (error_t err, dirvec_t ents) {

        if (unlikely(err)) {
          debug("Could not find: %s\n", name.c_str());
          callback(err, dirents);
          return;
        }

        // look for name in directory
        for (auto& e : *ents) {
          if (unlikely(e.name() == name)) {
            // go to this directory, unless its the last name
            debug("Found match for %s", name.c_str());
            // enter the matching directory
            debug("\t\t cluster: %llu\n", e.block);
            // only follow directories
            if (e.type() == DIR)
              (*next)(e.block);
            else
              callback({ error_t::E_NOTDIR, e.name() }, dirents);
            return;
          }
        } // for (ents)

        debug("NO MATCH for %s\n", name.c_str());
        callback({ error_t::E_NOENT, name }, dirents);
      });

    };
    // start by reading root directory
    (*next)(0);
  }

  void FAT::ls(const std::string& path, on_ls_func on_ls) {

    // parse this path into a stack of names
    auto pstk = std::make_shared<Path> (path);

    traverse(pstk,
    [on_ls] (error_t error, dirvec_t dirents) {
      on_ls(error, dirents);
    });
  }
  void FAT::ls(const Dirent& ent, on_ls_func on_ls)
  {
    auto dirents = std::make_shared<dirvector> ();
    // verify ent is a directory
    if (!ent.is_valid() || !ent.is_dir()) {
      on_ls( { error_t::E_NOTDIR, ent.name() }, dirents );
      return;
    }
    // convert cluster to sector
    uint32_t S = this->cl_to_sector(ent.block);
    // read result directory entries into ents
    int_ls(S, dirents,
    [on_ls] (error_t err, dirvec_t entries) {
      on_ls( err, entries );
    });
  }

  void FAT::read(const Dirent& ent, uint64_t pos, uint64_t n, on_read_func callback)
  {
    // when n=0 roundup() will return an invalid value
    if (n == 0) {
      callback({ error_t::E_IO, "Zero read length" }, buffer_t(), 0);
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
    device.read(this->cl_to_sector(ent.block) + sector, nsect,
    [pos, n, callback, internal_ofs] (buffer_t data) {

      if (!data) {
        // general I/O error occurred
        debug("Failed to read sector %u for read()", sector);
        callback({ error_t::E_IO, "Unable to read file" }, buffer_t(), 0);
        return;
      }

      // when the offset is non-zero we aren't on a sector boundary
      if (internal_ofs != 0) {
        // so, we need to copy offset data to data buffer
        auto* result = new uint8_t[n];
        memcpy(result, data.get() + internal_ofs, n);
        data = buffer_t(result, std::default_delete<uint8_t[]>());
      }

      callback(no_error, data, n);
    });
  }

  void FAT::stat(const std::string& strpath, on_stat_func func)
  {
    // manual lookup
    auto path = std::make_shared<Path> (strpath);
    if (unlikely(path->empty())) {
      // root doesn't have any stat anyways
      // Note: could use ATTR_VOLUME_ID in FAT
      func({ error_t::E_NOENT, "Cannot stat root" }, Dirent(INVALID_ENTITY, strpath));
      return;
    }

    debug("stat: %s\n", path->back().c_str());
    // extract file we are looking for
    std::string filename = path->back();
    path->pop_back();

    traverse(path,
    [this, filename, func] (error_t error, dirvec_t dirents)
    {
      if (unlikely(error)) {
        // no path, no file!
        func(error, Dirent(INVALID_ENTITY, filename));
        return;
      }

      // find the matching filename in directory
      for (auto& e : *dirents) {
        if (unlikely(e.name() == filename)) {
          // return this dir entry
          func(no_error, e);
          return;
        }
      }

      // not found
      func({ error_t::E_NOENT, filename }, Dirent(INVALID_ENTITY, filename));
    });
  }

  void FAT::cstat(const std::string& strpath, on_stat_func func)
  {
    // cache lookup
    auto it = stat_cache.find(strpath);
    if (it != stat_cache.end()) {
      debug("used cached stat for %s\n", strpath.c_str());
      func(no_error, it->second);
      return;
    }

    stat(strpath,
    [this, strpath, func] (error_t error, const File_system::Dirent& ent) {
        stat_cache[strpath] = ent;
        func(error, ent);
    });
  }
}
