#define DEBUG
#include <fs/fat.hpp>

#include <cassert>
#include <fs/mbr.hpp>
#include <fs/path.hpp>
#include <debug>

#include <cstring>
#include <memory>
#include <locale>
#include <kernel/syscalls.hpp> // for panic()

#include <info>

#define likely(x)       __builtin_expect(!!(x), 1)
#define unlikely(x)     __builtin_expect(!!(x), 0)

inline std::string trim_right_copy(
  const std::string& s,
  const std::string& delimiters = " \f\n\r\t\v" )
{
  return s.substr( 0, s.find_last_not_of( delimiters ) + 1 );
}

namespace fs
{
  typedef FileSystem::Buffer Buffer;
  
  Buffer FAT::read(const Dirent& ent, uint64_t pos, uint64_t n)
  {
    // cluster -> sector + position -> sector
    uint32_t sector = this->cl_to_sector(ent.block) + pos / this->sector_size;
    
    // the resulting buffer
    uint8_t* result = new uint8_t[n];
    
    // in all cases, read the first sector
    buffer_t data = device.read_sync(sector);
    
    uint32_t internal_ofs = pos % device.block_size();
    // keep track of total bytes
    uint64_t total = n;
    // calculate bytes to read before moving on to next sector
    uint32_t rest = device.block_size() - (pos - internal_ofs);
    
    // if what we want to read is larger than the rest, exit early
    if (rest > n)
    {
      memcpy(result, data.get() + internal_ofs, n);
      
      return Buffer(no_error, buffer_t(result), n);
    }
    // otherwise, read to the sector border
    uint8_t* ptr = result;
    memcpy(ptr, data.get() + internal_ofs, rest);
    ptr += rest;
    n   -= rest;
    sector += 1;
    
    // copy entire sectors
    while (n > device.block_size())
    {
      data = device.read_sync(sector);
      
      memcpy(ptr, data.get(), device.block_size());
      ptr += device.block_size();
      n   -= device.block_size();
      sector += 1;
    }
    
    // copy remainder
    if (likely(n > 0))
    {
      data = device.read_sync(sector);
      memcpy(ptr, data.get(), n);
    }
    
    return Buffer(no_error, buffer_t(result), total);
  }
  
  error_t FAT::int_ls(uint32_t sector, dirvec_t ents)
  {
    bool done = false;
    while (!done)
    {
      // read sector sync
      buffer_t data = device.read_sync(sector);
      if (!data) return true;
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
      if (unlikely(e.name() == name))
      {
        // go to this directory, unless its the last name
        debug("traverse_sync: Found match for %s", name.c_str());
        // enter the matching directory
        debug("\t\t cluster: %lu\n", e.block);
        // only follow if the name is a directory
        if (e.type() == DIR)
        {
          found = e;
          break;
        }
        else
        {
          // not dir = error, for now
          return true;
        }
      } // for (ents)
      
      // validate result
      if (found.type() == INVALID_ENTITY)
      {
        debug("traverse_sync: NO MATCH for %s\n", name.c_str());
        return true;
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
