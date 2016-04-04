//#define DEBUG
#include <fs/fat.hpp>

#include <cassert>
#include <fs/path.hpp>
#include <debug>

#include <cstring>
#include <memory>
#include <locale>

#define likely(x)       __builtin_expect(!!(x), 1)
#define unlikely(x)     __builtin_expect(!!(x), 0)

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
    *next = 
      [this, sector, callback, dirents, next] (uint32_t sector)
      {
        debug("int_ls: sec=%u\n", sector);
        device.read(sector,
                    [this, sector, callback, dirents, next] (buffer_t data)
      {
        if (!data)
          {
            // could not read sector
            callback(true, dirents);
            return;
          }
        
        // parse entries in sector
        bool done = int_dirent(sector, data.get(), dirents);
        if (done)
          {
            // execute callback
            callback(no_error, dirents);
          }
        else
          {
            // go to next sector
            (*next)(sector+1);
          }
        
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
    *next = 
      [this, path, next, callback] (uint32_t cluster)
      {
        if (path->empty())
          {
            // attempt to read directory
            uint32_t S = this->cl_to_sector(cluster);
        
            // result allocated on heap
            auto dirents = std::make_shared<std::vector<Dirent>> ();
        
            int_ls(S, dirents,
                   [callback] (error_t error, dirvec_t ents)
      {
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
      
        // list directory contents
        int_ls(S, dirents,
               [name, dirents, next, callback] (error_t error, dirvec_t ents)
      {
        if (unlikely(error))
          {
            debug("Could not find: %s\n", name.c_str());
            callback(true, dirents);
            return;
          }
        
        // look for name in directory
        for (auto& e : *ents)
          {
            if (unlikely(e.name() == name))
              {
                // go to this directory, unless its the last name
                debug("Found match for %s", name.c_str());
                // enter the matching directory
                debug("\t\t cluster: %llu\n", e.block);
                // only follow directories
                if (e.type() == DIR)
                  (*next)(e.block);
                else
                  callback(true, dirents);
                return;
              }
          } // for (ents)
        
        debug("NO MATCH for %s\n", name.c_str());
        callback(true, dirents);
      });
      
      };
    // start by reading root directory
    (*next)(0);
  }
  
  void FAT::ls(const std::string& path, on_ls_func on_ls)
  {
    // parse this path into a stack of names
    auto pstk = std::make_shared<Path> (path);
    
    traverse(pstk, 
             [on_ls] (error_t error, dirvec_t dirents)
             {
               on_ls(error, dirents);
             });
  }
  
  void FAT::read(const Dirent& ent, uint64_t pos, uint64_t n, on_read_func callback)
  {
    // cluster -> sector
    uint32_t sector = this->cl_to_sector(ent.block);
    
    // allocate buffer
    auto* buffer = new uint8_t[n];
    
    // start reading process
    typedef std::function<void(size_t, size_t, size_t)> next_func_t;
    auto next = std::make_shared<next_func_t> ();
    
    *next = 
      [this, sector, buffer, callback, next] (size_t start, size_t n, size_t end)
      {
        if (unlikely(n == end))
          {
            // report back to HQ
            debug("DONE start=%lu, current=%lu, total=%lu\n", start, n, end - start);
            // create shared buffer
            auto buffer_ptr = buffer_t(buffer, std::default_delete<uint8_t[]>());
            // notify caller
            callback(no_error, buffer_ptr, end - start);
            return;
          }
      
        // read the current sector based on position @n
        uint32_t current_sector = sector + n / this->sector_size;
      
        device.read(current_sector,
                    [this, start, n, end, buffer, &callback, sector, next] (buffer_t data)
      {
        if (!data)
          {
            // general I/O error occurred
            debug("Failed to read sector %u for read()", sector);
            // cleanup
            delete[] buffer;
            callback(true, buffer_t(), 0);
            return;
          }
        
        uint32_t length = n & (sector_size-1);
        if (n == start && n > 0)
          {
            length = sector_size - length;
          }
        else
          {
            length = (n + sector_size) < end ? sector_size : (end - n);
          }
        
        // copy over data
        memcpy(buffer + n, data.get(), length);
        // continue reading next sector
        (*next)(start, n + length, end);
      });
      };
    
    // start!
    (*next)(pos, pos, pos + n);
  }
  
  void FAT::readFile(const std::string& strpath, on_read_func callback)
  {
    auto path = std::make_shared<Path> (strpath);
    if (unlikely(path->empty()))
      {
        // there is no possible file to read where path is empty
        callback(true, nullptr, 0);
        return;
      }
    debug("readFile: %s\n", path->back().c_str());
    
    std::string filename = path->back();
    path->pop_back();
    
    traverse(path,
             [this, filename, &callback] (error_t error, dirvec_t dirents)
             {
               if (unlikely(error))
                 {
                   // no path, no file!
                   callback(error, buffer_t(), 0);
                   return;
                 }
      
               // find the matching filename in directory
               for (auto& ent : *dirents)
                 {
                   if (unlikely(ent.name() == filename))
                     {
                       // read this file
                       read(ent, 0, ent.size, callback);
                       return;
                     }
                 }
      
               // file not found
               callback(true, buffer_t(), 0);
             });
  } // readFile()
  
  void FAT::stat(const std::string& strpath, on_stat_func func)
  {
    auto path = std::make_shared<Path> (strpath);
    if (unlikely(path->empty()))
      {
        // root doesn't have any stat anyways
        // Note: could use ATTR_VOLUME_ID in FAT
        func(true, Dirent(INVALID_ENTITY, strpath));
        return;
      }
    
    debug("stat: %s\n", path->back().c_str());
    // extract file we are looking for
    std::string filename = path->back();
    path->pop_back();
    // we need to remember this later
    auto callback = std::make_shared<on_stat_func> (func);
    
    traverse(path,
             [this, filename, callback] (error_t error, dirvec_t dirents)
             {
               if (unlikely(error))
                 {
                   // no path, no file!
                   (*callback)(error, Dirent(INVALID_ENTITY, filename));
                   return;
                 }
      
               // find the matching filename in directory
               for (auto& e : *dirents)
                 {
                   if (unlikely(e.name() == filename))
                     {
                       // read this file
                       (*callback)(no_error, e);
                       return;
                     }
                 }
      
               // not found
               (*callback)(true, Dirent(INVALID_ENTITY, filename));
             });
  }
}
