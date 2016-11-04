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

#pragma once
#ifndef FS_FAT_HPP
#define FS_FAT_HPP

#include "filesystem.hpp"
#include <hw/drive.hpp>
#include <functional>
#include <cstdint>
#include <memory>
#include <map>

namespace fs
{
  class Path;

  struct FAT : public File_system
  {
    /// ----------------------------------------------------- ///
    void mount(uint64_t lba, uint64_t size, on_mount_func on_mount) override;

    // path is a path in the mounted filesystem
    void  ls     (const std::string& path, on_ls_func) override;
    void  ls     (const Dirent& entry,     on_ls_func) override;
    List  ls(const std::string& path) override;
    List  ls(const Dirent&) override;

    /** Read @n bytes from file pointed by @entry starting at position @pos */
    void   read(const Dirent&, uint64_t pos, uint64_t n, on_read_func) override;
    Buffer read(const Dirent&, uint64_t pos, uint64_t n) override;

    // return information about a filesystem entity
    void   stat(const std::string&, on_stat_func) override;
    Dirent stat(const std::string& ent) override;
    // async cached stat
    void   cstat(const std::string&, on_stat_func) override;

    // returns the name of the filesystem
    std::string name() const override
    {
      switch (this->fat_type)
        {
        case T_FAT12:
          return "FAT12";
        case T_FAT16:
          return "FAT16";
        case T_FAT32:
          return "FAT32";
        }
      return "Invalid fat type";
    }
    /// ----------------------------------------------------- ///

    // constructor
    FAT(hw::Drive& dev);
    virtual ~FAT() = default;

  private:
    // FAT types
    static const int T_FAT12 = 0;
    static const int T_FAT16 = 1;
    static const int T_FAT32 = 2;

    // Attribute masks
    static const uint8_t ATTR_READ_ONLY = 0x01;
    static const uint8_t ATTR_HIDDEN    = 0x02;
    static const uint8_t ATTR_SYSTEM    = 0x04;
    static const uint8_t ATTR_VOLUME_ID = 0x08;
    static const uint8_t ATTR_DIRECTORY = 0x10;
    static const uint8_t ATTR_ARCHIVE   = 0x20;

    // Mask for the last longname entry
    static const uint8_t LAST_LONG_ENTRY = 0x40;

    struct cl_dir
    {
      uint8_t  shortname[11];
      uint8_t  attrib;
      uint8_t  pad1[8];
      uint16_t cluster_hi;
      uint32_t modified;
      uint16_t cluster_lo;
      uint32_t filesize;

      bool is_longname() const
      {
        return (attrib & 0x0F) == 0x0F;
      }

      uint32_t dir_cluster(uint32_t root_cl) const
      {
        uint32_t cl = cluster_lo | (cluster_hi << 16);
        return (cl) ? cl : root_cl;

      }

      Enttype type() const
      {
        if (attrib & ATTR_VOLUME_ID)
          return VOLUME_ID;
        else if (attrib & ATTR_DIRECTORY)
          return DIR;
        else
          return FILE;
      }

      uint32_t size() const
      {
        return filesize;
      }

    } __attribute__((packed));

    struct cl_long
    {
      uint8_t  index;
      uint16_t first[5];
      uint8_t  attrib;
      uint8_t  entry_type;
      uint8_t  checksum;
      uint16_t second[6];
      uint16_t zero;
      uint16_t third[2];

      // the index value for this long entry
      // starting with the highest (hint: read manual)
      uint8_t long_index() const
      {
        return index & ~0x40;
      }
      // true if this is the last long index
      uint8_t is_last() const
      {
        return (index & LAST_LONG_ENTRY) != 0;
      }
    } __attribute__((packed));

    // helper functions
    uint32_t cl_to_sector(uint32_t const cl)
    {
      if (cl <= 2)
        return lba_base + data_index + (this->root_cluster - 2) * sectors_per_cluster - this->root_dir_sectors;
      else
        return lba_base + data_index + (cl - 2) * sectors_per_cluster;
    }

    uint16_t cl_to_entry_offset(uint32_t cl)
    {
      if (fat_type == T_FAT16)
        return (cl * 2) % sector_size;
      else // T_FAT32
        return (cl * 4) % sector_size;
    }
    uint16_t cl_to_entry_sector(uint32_t cl)
    {
      if (fat_type == T_FAT16)
        return reserved + (cl * 2 / sector_size);
      else // T_FAT32
        return reserved + (cl * 4 / sector_size);
    }

    // initialize filesystem by providing base sector
    void init(const void* base_sector);
    // return a list of entries from directory entries at @sector
    typedef delegate<void(error_t, dirvec_t)> on_internal_ls_func;
    void int_ls(uint32_t sector, dirvec_t, on_internal_ls_func);
    bool int_dirent(uint32_t sector, const void* data, dirvector&);

    // tree traversal
    typedef delegate<void(error_t, dirvec_t)> cluster_func;
    // async tree traversal
    void traverse(std::shared_ptr<Path> path, cluster_func callback);
    // sync version
    error_t traverse(Path path, dirvector&);
    error_t int_ls(uint32_t sector, dirvector&);

    // device we can read and write sectors to
    hw::Drive& device;

    /// private members ///
    // the location of this partition
    uint32_t lba_base;
    // the size of this partition
    uint32_t lba_size;

    uint16_t sector_size; // from bytes_per_sector
    uint32_t sectors;   // total sectors in partition
    uint32_t clusters;  // number of indexable FAT clusters

    uint8_t  fat_type;  // T_FAT12, T_FAT16 or T_FAT32
    uint16_t reserved;  // number of reserved sectors

    uint32_t sectors_per_fat;
    uint16_t sectors_per_cluster;
    uint16_t root_dir_sectors; // FAT16 root entries

    uint32_t root_cluster;  // index of root cluster
    uint32_t data_index;    // index of first data sector (relative to partition)
    uint32_t data_sectors;  // number of data sectors

    // simplistic cache for stat results
    std::map<std::string, File_system::Dirent> stat_cache;
  };

} // fs

#endif
