#include <fs/fat.hpp>

#include <cassert>
#include <fs/mbr.hpp>
#include <fs/path.hpp>
#include <debug>

#include <cstring>
#include <memory>
#include <locale>

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
  FAT32::FAT32(std::shared_ptr<IDiskDevice> dev)
    : device(dev)
  {
    
  }
  
  void FAT32::init(const void* base_sector)
  {
    // assume its the master boot record for now
    auto* mbr = (MBR::mbr*) base_sector;
    
    MBR::BPB* bpb = mbr->bpb();
    this->sector_size = bpb->bytes_per_sector;
    
    // Let's begin our incantation
    // To drive out the demons of old DOS we have to read some PBP values
    debug("Bytes per sector: \t%u\n", bpb->bytes_per_sector);
    debug("Sectors per cluster: \t%u\n", bpb->sectors_per_cluster);
    debug("Reserved sectors: \t%u\n", bpb->reserved_sectors);
    debug("Number of FATs: \t%u\n", bpb->fa_tables);
    
    debug("Small sectors (FAT16): \t%u\n", bpb->small_sectors);
    
    debug("Sectors per FAT: \t%u\n", bpb->sectors_per_fat);
    debug("Sectors per Track: \t%u\n", bpb->sectors_per_track);
    debug("Number of Heads: \t%u\n", bpb->num_heads);
    debug("Hidden sectors: \t%u\n", bpb->hidden_sectors);
    debug("Large sectors: \t%u\n", bpb->large_sectors);
    debug("Disk number: \t0x%x\n", bpb->disk_number);
    debug("Signature: \t0x%x\n", bpb->signature);
    
    debug("System ID: \t%.8s\n", bpb->system_id);
    
    // initialize FAT
    if (bpb->small_sectors) // FAT16
    {
        this->fat_type  = FAT32::T_FAT16;
        this->sectors = bpb->small_sectors;
        this->sectors_per_fat = bpb->sectors_per_fat;
        this->root_dir_sectors = ((bpb->root_entries * 32) + (sector_size - 1)) / sector_size;
        debug("Root dir sectors: %u\n", this->root_dir_sectors);
    }
    else
    {
        this->fat_type   = FAT32::T_FAT32;
        this->sectors = bpb->large_sectors;
        this->sectors_per_fat = *(uint32_t*) &mbr->boot[25];
        this->root_dir_sectors = 0;
    }
    // calculate index of first data sector
    this->data_index = bpb->reserved_sectors + (bpb->fa_tables * this->sectors_per_fat) + this->root_dir_sectors;
    debug("First data sector: %u\n", this->data_index);
    // number of reserved sectors is needed constantly
    this->reserved = bpb->reserved_sectors;
    debug("Reserved sectors: %u\n", this->reserved);
    // number of sectors per cluster is important for calculating entry offsets
    this->sectors_per_cluster = bpb->sectors_per_cluster;
    debug("Sectors per cluster: %u\n", this->sectors_per_cluster);
    // calculate number of data sectors
    this->data_sectors = this->sectors - this->data_index;
    debug("Data sectors: %u\n", this->data_sectors);
    // calculate total cluster count
    this->clusters = this->data_sectors / this->sectors_per_cluster;
    debug("Total clusters: %u\n", this->clusters);
    
    // now that we're here, we can determine the actual FAT type
    // using the official method:
    if (this->clusters < 4085)
    {
      this->fat_type = FAT32::T_FAT12;
      this->root_cluster = 2;
      debug("The image is type FAT12, with %u clusters\n", this->clusters);
    }
    else if (this->clusters < 65525)
    {
      this->fat_type = FAT32::T_FAT16;
      this->root_cluster = 2;
      debug("The image is type FAT16, with %u clusters\n", this->clusters);
    }
    else
    {
      this->fat_type = FAT32::T_FAT32;
      this->root_cluster = *(uint32_t*) &mbr->boot[33];
      debug("The image is type FAT32, with %u clusters\n", this->clusters);
    }
    debug("Root cluster index: %u\n", this->root_cluster);
    debug("System ID: %.8s\n", bpb->system_id);
  }
  
  void FAT32::mount(uint8_t partid, on_mount_func on_mount)
  {
    // read Master Boot Record (sector 0)
    device->read_sector(0,
    [this, partid, on_mount] (const void* data)
    {
      auto* mbr = (MBR::mbr*) data;
      assert(mbr != nullptr);
      
      // verify image signature
      debug("OEM name: \t%s\n", mbr->oem_name);
      debug("MBR signature: \t0x%x\n", mbr->magic);
      assert(mbr->magic == 0xAA55);
      
      /// the mount partition id tells us the LBA offset to the volume
      // assume MBR for now
      assert(partid == 0);
      
      // initialize FAT16 or FAT32 filesystem
      init(mbr);
      
      // determine which FAT version is mounted
      switch (this->fat_type)
      {
      case FAT32::T_FAT12:
          printf("--> Mounting FAT12 filesystem\n");
          break;
      case FAT32::T_FAT16:
          printf("--> Mounting FAT16 filesystem\n");
          break;
      case FAT32::T_FAT32:
          printf("--> Mounting FAT32 filesystem\n");
          break;
      }
      
      // on_mount callback
      on_mount(true);
    });
  }
  
  bool FAT32::int_dirent(
      uint32_t  sector,
      const void* data, 
      dirvec_t dirents)
  {
      auto* root = (FAT32::cl_dir*) data;
      bool  found_last = false;
      
      for (int i = 0; i < 16; i++)
      {
        if (unlikely(root[i].shortname[0] == 0x0))
        {
          //printf("end of dir\n");
          found_last = true;
          // end of directory
          break;
        }
        else if (unlikely(root[i].shortname[0] == 0xE5))
        {
          // unused index
        }
        else
        {
            // traverse long names, then final cluster
            // to read all the relevant info
            
            if (likely(root[i].is_longname()))
            {
              auto* L = (FAT32::cl_long*) &root[i];
              // the last long index is part of a chain of entries
              if (L->is_last())
              {
                // buffer for long filename
                char final_name[256];
                int  final_count = 0;
                
                int  total = L->long_index();
                // go to the last entry and work backwards
                i += total-1;
                L += total-1;
                
                for (int idx = total; idx > 0; idx--)
                {
                  uint16_t longname[13];
                  memcpy(longname+ 0, L->first, 10);
                  memcpy(longname+ 5, L->second, 12);
                  memcpy(longname+11, L->third, 4);
                  
                  for (int j = 0; j < 13; j++)
                  {
                    // 0xFFFF indicates end of name
                    if (unlikely(longname[j] == 0xFFFF)) break;
                    // sometimes, invalid stuff are snuck into filenames
                    if (unlikely(longname[j] == 0x0)) break;
                    
                    final_name[final_count] = longname[j] & 0xFF;
                    final_count++;
                  }
                  L--;
                  
                  if (unlikely(final_count > 240))
                  {
                    debug("Suspicious long name length, breaking...\n");
                    break;
                  }
                }
                
                final_name[final_count] = 0;
                //printf("Long name: %s\n", final_name);
                
                i++; // skip over the long version
                // to the short version for the stats and cluster
                auto* D = &root[i];
                std::string dirname(final_name, final_count);
                dirname = trim_right_copy(dirname);
                
                dirents->emplace_back(
                  dirname, 
                  D->type(), 
                  D->dir_cluster(root_cluster), 
                  sector, // parent block
                  D->size(), 
                  D->attrib);
              }
            }
            else
            {
              auto* D = &root[i];
              //printf("Short name: %.11s\n", D->shortname);
              std::string dirname((char*) D->shortname, 11);
              dirname = trim_right_copy(dirname);
              
              dirents->emplace_back(
                dirname, 
                D->type(), 
                D->dir_cluster(root_cluster), 
                sector, // parent block
                D->size(), 
                D->attrib);
            }
        }
      } // directory list
      
      return found_last;
  }
  
  void FAT32::int_ls(
      uint32_t sector, 
      dirvec_t dirents, 
      on_internal_ls_func callback)
  {
    std::function<void(uint32_t)> next;
    
    next = [this, sector, callback, &dirents, next] (uint32_t sector)
    {
      //printf("int_ls: sec=%u\n", sector);
      device->read_sector(sector,
      [this, sector, callback, &dirents, next] (const void* data)
      {
        if (!data)
        {
          // could not read sector
          callback(false, dirents);
          return;
        }
        
        // parse entries in sector
        bool done = int_dirent(sector, data, dirents);
        if (done)
        {
          // execute callback
          callback(true, dirents);
        }
        else
        {
          // go to next sector
          next(sector+1);
        }
        
      }); // read root dir
    };
    
    // start reading sectors asynchronously
    next(sector);
  }
  
  void FAT32::traverse(std::shared_ptr<Path> path, cluster_func callback)
  {
    typedef std::function<void(uint32_t)> next_func_t;
    
    // parse this path into a stack of names
    debug("TRAVERSE: %s\n", path->to_string().c_str());
    
    // asynch stack traversal
    next_func_t next;
    next = 
    [this, path, &next, callback] (uint32_t cluster)
    {
      debug("Traversed to cluster %u\n", cluster);
      if (path->empty())
      {
        // attempt to read directory
        uint32_t S = this->cl_to_sector(cluster);
        //debug("Reading cluster %u at sector %u\n", cluster, S);
        
        // result allocated on heap
        auto dirents = std::make_shared<std::vector<Dirent>> ();
        
        int_ls(S, dirents,
        [S, callback] (error_t error, dirvec_t ents)
        {
          callback(error, S, ents);
        });
        return;
      }
      
      // retrieve next name
      std::string name = path->front();
      path->pop_front();
      
      debug("Current target: %s\n", name.c_str());
      uint32_t S = this->cl_to_sector(cluster);
      
      // result allocated on heap
      auto dirents = std::make_shared<std::vector<Dirent>> ();
      
      // list directory contents
      int_ls(S, dirents,
      [name, dirents, &next, S, callback] (error_t error, dirvec_t ents)
      {
        if (unlikely(!error))
        {
          debug("Could not find: %s\n", name.c_str());
          callback(false, S, dirents);
          return;
        }
        
        // look for name in directory
        for (auto& e : *ents)
        {
          if (unlikely(e.name == name))
          {
            // go to this directory, unless its the last name
            debug("Found match for %s", name.c_str());
            // enter the matching directory
            debug("\t\t cluster: %lu\n", e.block);
            // only follow directories
            if (e.type == DIR)
              next(e.block);
            else
              callback(false, S, dirents);
            return;
          }
        } // for (ents)
        
        debug("NO MATCH for %s\n", name.c_str());
        callback(false, S, dirents);
      });
      
    };
    
    // start by reading root directory
    next(this->root_cluster);
  }
  
  void FAT32::ls(const std::string& path, on_ls_func on_ls)
  {
    // parse this path into a stack of names
    auto pstk = std::make_shared<Path> (path);
    
    traverse(pstk, 
    [on_ls] (error_t error, uint32_t, dirvec_t dirents)
    {
      on_ls(error, dirents);
    });
  }
  
  void FAT32::readFile(const Dirent& ent, on_read_func callback)
  {
    // cluster -> sector
    uint32_t sector = this->cl_to_sector(ent.block);
    // number of sectors to read ahead
    size_t chunks = ent.size / sector_size + 1;
    // allocate buffer
    uint8_t* buffer = new uint8_t[chunks * sector_size];
    // at which sector we will stop
    size_t total   = chunks;
    size_t current = 0;
    
    typedef std::function<void(uint32_t, size_t, size_t)> next_func_t;
    auto* next = new next_func_t;
    
    *next = 
    [this, buffer, ent, callback, next] (uint32_t sector, size_t current, size_t total)
    {
      if (unlikely(current == total))
      {
        // report back to HQ
        debug("DONE SIZE: %lu  (current=%lu, total=%lu)\n", 
            ent.size, current, total);
        callback(true, buffer, ent.size);
        // cleanup (after callback)
        delete next;
        delete[] buffer;
        return;
      }
      device->read_sector(sector,
      [this, current, total, buffer, ent, &callback, sector, next] (const void* data)
      {
        if (!data)
        {
          debug("Failed to read sector %u for read()", sector);
          // cleanup
          delete next;
          delete[] buffer;
          callback(false, nullptr, 0);
          return;
        }
        
        // copy over data
        memcpy(buffer + current * sector_size, data, sector_size);
        // continue reading next sector
        (*next)(sector+1, current+1, total);
      });
    };
    
    // start!
    (*next)(sector, current, total);
  }
  
  void FAT32::readFile(const std::string& strpath, on_read_func callback)
  {
    auto path = std::make_shared<Path> (strpath);
    if (unlikely(path->empty()))
    {
      // there is no possible file to read where path is empty
      callback(false, nullptr, 0);
      return;
    }
    debug("readFile: %s\n", path->back().c_str());
    
    std::string filename = path->back();
    path->pop_back();
    
    traverse(path,
    [this, filename, &callback] (error_t error, uint32_t, dirvec_t dirents)
    {
      if (unlikely(!error))
      {
        // no path, no file!
        callback(false, nullptr, 0);
        return;
      }
      
      // find the matching filename in directory
      for (auto& e : *dirents)
      {
        if (unlikely(e.name == filename))
        {
          // read this file
          readFile(e, callback);
          return;
        }
      }
    });
  } // readFile()
  
  void FAT32::stat(const std::string& strpath, on_stat_func callback)
  {
    auto path = std::make_shared<Path> (strpath);
    if (unlikely(path->empty()))
    {
      // root doesn't have any stat anyways (except ATTR_VOLUME_ID in FAT)
      callback(false, Dirent());
      return;
    }
    
    debug("stat: %s\n", path->back().c_str());
    // extract file we are looking for
    std::string filename = path->back();
    path->pop_back();
    
    traverse(path,
    [this, filename, &callback] (error_t error, uint32_t, dirvec_t dirents)
    {
      if (unlikely(!error))
      {
        // no path, no file!
        callback(false, Dirent());
        return;
      }
      
      // find the matching filename in directory
      for (auto& e : *dirents)
      {
        if (unlikely(e.name == filename))
        {
          // read this file
          callback(true, e);
          return;
        }
      }
      
      // not found
      callback(false, Dirent());
    });
  }
}
