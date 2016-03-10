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

#include "common.hpp"
#include <hw/disk_device.hpp>

#include <deque>
#include <vector>
#include <functional>

namespace fs {

class FileSystem; //< FileSystem interface

template <typename FS>
class Disk {
public:
  struct Partition; //< Representation of a disk partition

  /** Callbacks */
  using on_parts_func = std::function<void(error_t, std::vector<Partition>&)>;
  using on_mount_func = std::function<void(error_t)>;
  
  /** Constructor */
  explicit Disk(hw::IDiskDevice&);

  enum partition_t {
    MBR = 0, //< Master Boot Record (0)
    /** extended partitions (1-4) */
    VBR1,
    VBR2,
    VBR3,
    VBR4,
    
    INVALID
  }; //<  enum partition_t
  
  struct Partition {
    explicit Partition(const uint8_t  fl,  const uint8_t  Id,
                       const uint32_t LBA, const uint32_t sz) noexcept :
      flags     {fl},
      id        {Id},
      lba_begin {LBA},
      sectors   {sz}
    {}
    
    uint8_t  flags;
    uint8_t  id;
    uint32_t lba_begin;
    uint32_t sectors;
    
    // true if the partition has boot code / is bootable
    bool is_boot() const noexcept
    { return flags & 0x1; }
    
    // human-readable name of partition id
    std::string name() const;
    
    // logical block address of beginning of partition
    uint32_t lba() const
    { return lba_begin; }
    
  }; //< struct Partition
  
  /** Return a reference to the specified filesystem <FS> */
  FileSystem& fs() noexcept
  { return *filesys; }
  
  //************** disk functions **************//
  
  hw::IDiskDevice& dev() noexcept
  { return device; }
  
  // Returns true if the disk has no sectors
  bool empty() const noexcept
  { return device.size() == 0; }
  
  // Mounts the first partition detected (MBR -> VBR1-4 -> ext)
  void mount(on_mount_func func);
  
  // Mount partition @part as the filesystem FS
  void mount(partition_t part, on_mount_func func);
  
  /**
   *  Returns a vector of the partitions on a disk
   *
   *  The disk does not need to be mounted beforehand
   */
  void partitions(on_parts_func func);
  
private:
  hw::IDiskDevice& device;
  std::unique_ptr<FS> filesys;
}; //< class Disk

template <typename FS>
inline Disk<FS>::Disk(hw::IDiskDevice& dev) :
  device {dev}
{
  filesys.reset(new FS(device));
}

} //< namespace fs

#include "disk.inl"

#endif //< FS_DISK_HPP
