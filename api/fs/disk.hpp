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

/**
 * /// Create basic FAT disk ///
 *
 * // create disk from a given disk-device
 * auto disk = std::make_shared<Disk> (device);
 * // mount filesystem on auto-detected volume
 * disk->mount(
 * [disk] (fs::error_t err) {
 *   if (err) {
 *     printf("Bad!\n");
 *     return;
 *   }
 *   // reference to filesystem
 *   auto& fs = disk->fs();
 *   // synchronous stat:
 *   auto dirent = fs.stat("/file");
 * });
 *
 * /// Construct custom filesystem ///
 *
 * // constructing on MBR means mount on sector 0
 * disk->mount<MyFile_system>(filesystem_args..., Disk::MBR,
 * [disk] {
 *   printf("Disk mounted!\n");
 *   auto& fs = disk->fs();
 *
 *   auto dirent = fs.stat("/file");
 * });
 *
**/

#include "common.hpp"
#include "filesystem.hpp"
#include <hw/drive.hpp>

#include <deque>
#include <vector>
#include <functional>

namespace fs {

  class Disk {
  public:
    struct Partition;
    using on_parts_func = std::function<void(error_t, std::vector<Partition>&)>;
    using on_mount_func = std::function<void(error_t)>;
    using lba_t = uint32_t;

    enum partition_t {
      MBR = 0, //< Master Boot Record (0)
      VBR1,   //> extended partitions (1-4)
      VBR2,
      VBR3,
      VBR4,
    };

    struct Partition {
      explicit Partition(const uint8_t  fl,  const uint8_t  Id,
                         const uint32_t LBA, const uint32_t sz) noexcept
      : flags     {fl},
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

    //************** disk functions **************//

    // construct a disk with a given disk-device
    explicit Disk(hw::Drive&);

    // returns the device the disk is using
    hw::Drive& dev() noexcept
    { return device; }

    // Returns true if the disk has no sectors
    bool empty() const noexcept
    { return device.size() == 0; }

    // Mounts the first partition detected (MBR -> VBR1-4 -> ext)
    // NOTE: Always detects and instantiates a FAT filesystem
    void mount(on_mount_func func);

    // Mounts specified partition
    // NOTE: Always detects and instantiates a FAT filesystem
    void mount(partition_t part, on_mount_func func);

    // mount custom filesystem on MBR or VBRn
    template <class T, class... Args>
    void mount(Args&&... args, partition_t part, on_mount_func func)
    {
      // construct custom filesystem
      filesys.reset(new T(args...));
      internal_mount(part, func);
    }

    // Creates a vector of the partitions on disk (see: on_parts_func)
    // Note: The disk does not need to be mounted beforehand
    void partitions(on_parts_func func);

    // returns true if a filesystem is mounted
    bool fs_mounted() const noexcept
    { return (bool) filesys; }

    // Returns a reference to a mounted filesystem
    // If no filesystem is mounted, the results are undefined
    File_system& fs() noexcept
    { return *filesys; }

  private:
    void internal_mount(partition_t part, on_mount_func func);

    hw::Drive& device;
    std::unique_ptr<File_system> filesys;
  }; //< class Disk

  using Disk_ptr = std::shared_ptr<Disk>;

} //< namespace fs

#endif //< FS_DISK_HPP
