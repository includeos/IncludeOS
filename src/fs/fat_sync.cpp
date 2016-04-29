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
  typedef FileSystem::Buffer Buffer;
  
  Buffer FAT::read(const Dirent& ent, uint64_t pos, uint64_t n)
  {
    // cluster -> sector + position
    uint32_t sector = pos / this->sector_size;
    uint32_t nsect = roundup(pos + n, sector_size) / sector_size - sector;
    
    // the resulting buffer
    uint8_t* result = new uint8_t[n];
    
    // read @nsect sectors ahead
    buffer_t data = device.read_sync(this->cl_to_sector(ent.block) + sector, nsect);
    // where to start copying from the device result
    uint32_t internal_ofs = pos % device.block_size();
    // copy data to result buffer
    memcpy(result, data.get() + internal_ofs, n);
    auto buffer = buffer_t(result, std::default_delete<uint8_t[]>());
    
    return Buffer(no_error, buffer, n);
  }
  
  Buffer FAT::readFile(const std::string& strpath)
  {
    Path path(strpath);
    if (unlikely(path.empty())) {
      // there is no possible file to read where path is empty
      return Buffer({ error_t::E_NOENT, "Path is empty" }, nullptr, 0);
    }
    debug("readFile: %s\n", path.back().c_str());
    
    std::string filename = path.back();
    path.pop_back();
    
    // result directory entries are put into @dirents
    auto dirents = new_shared_vector();
    
    auto err = traverse(path, dirents);
    if (err) return Buffer(err, buffer_t(), 0); // for now
    
    // find the matching filename in directory
    for (auto& e : *dirents)
    if (unlikely(e.name() == filename)) {
      // read this file
      return read(e, 0, e.size);
    }
    // entry not found
    return Buffer({ error_t::E_NOENT, filename }, buffer_t(), 0);
  } // readFile()
  
  error_t FAT::int_ls(uint32_t sector, dirvec_t ents)
  {
    bool done = false;
    while (!done) {
      // read sector sync
      buffer_t data = device.read_sync(sector);
      if (!data) return { error_t::E_IO, "Unable to read directory" };
      // parse directory into @ents
      done = int_dirent(sector, data.get(), ents);
      // go to next sector until done
      sector++;
    }
    return no_error;
  }
  
  error_t FAT::traverse(Path path, dirvec_t ents)
  {
    // start with root dir
    uint32_t cluster = 0;
    // directory entries are read into this
    auto dirents = new_shared_vector();
    Dirent found(INVALID_ENTITY);
    
    while (!path.empty())
      {
        uint32_t S = this->cl_to_sector(cluster);
        dirents->clear(); // mui importante
        // sync read entire directory
        auto err = int_ls(S, dirents);
        if (err) return err;
        // the name we are looking for
        std::string name = path.front();
        path.pop_front();
      
        // check for matches in dirents
        for (auto& e : *dirents)
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
  
  error_t FAT::ls(const std::string& strpath, dirvec_t ents)
  {
    return traverse(strpath, ents);
  }
  
  FAT::Dirent FAT::stat(const std::string& strpath)
  {
    Path path(strpath);
    if (unlikely(path.empty()))
      {
        // root doesn't have any stat anyways (except ATTR_VOLUME_ID in FAT)
        return Dirent(INVALID_ENTITY);
      }
    
    debug("stat_sync: %s\n", path.back().c_str());
    // extract file we are looking for
    std::string filename = path.back();
    path.pop_back();
    
    // result directory entries are put into @dirents
    auto dirents = std::make_shared<std::vector<Dirent>> ();
    
    auto err = traverse(path, dirents);
    if (err) return Dirent(INVALID_ENTITY); // for now
    
    // find the matching filename in directory
    for (auto& e : *dirents)
      {
        if (unlikely(e.name() == filename))
          {
            // return this directory entry
            return e;
          }
      }
    // entry not found
    return Dirent(INVALID_ENTITY);
  }
}
