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
#ifndef FS_DISK_HPP
#define FS_DISK_HPP

#include <functional>
#include <deque>
#include <memory>
#include <vector>
#include "disk_device.hpp"
#include "error.hpp"

namespace fs
{
  class FileSystem;
  
  template <typename FS>
  class Disk
  {
  public:
    enum partition_t
    {
      // master boot record (0)
      PART_MBR = 0,
      // extended partitions (1-4)
      PART_VBR1,
      PART_VBR2,
      PART_VBR3,
      PART_VBR4
    };
    
    struct Partition
    {
      Partition(uint8_t fl, uint8_t Id, uint32_t LBA, uint32_t sz)
          : flags(fl), id(Id), lba_begin(LBA), sectors(sz) {}
      
      uint8_t  flags;
      uint8_t  id;
      uint32_t lba_begin;
      uint32_t sectors;
      
      bool is_boot() const noexcept
      {
        return flags & 0x1;
      }
      
      std::string name() const;
    };
    
    /// callbacks  ///
    typedef std::function<void(error_t, std::vector<Partition>&)> on_parts_func;
    typedef std::function<void(error_t)> on_mount_func;
    
    /// initialization ///
    Disk(IDiskDevice& dev);
    
    /// return a reference to the specified filesystem <FS>
    FileSystem& fs()
    {
      return *filesys;
    }
    
    /// disk functions ///
    // mount selected filesystem
    void mount(partition_t part, on_mount_func func);
    
    // returns a vector of the partitions on a disk
    // the disk does not need to be mounted beforehand
    void partitions(on_parts_func func);
    
  private:
    IDiskDevice& device;
    std::unique_ptr<FS> filesys;
  };
  
  template <typename FS>
  Disk<FS>::Disk(IDiskDevice& dev)
    : device(dev)
  {
    filesys.reset(new FS(device));
  }
  
} // fs

#include "disk.inl"

#endif
