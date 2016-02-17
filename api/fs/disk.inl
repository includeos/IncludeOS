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

#include "mbr.hpp"

namespace fs {

template <typename FS>
void Disk<FS>::partitions(on_parts_func func) {
  /** Read Master Boot Record (sector 0) */
  device.read_sector(0,
  [this, func] (const void* data)
  {
    std::vector<Partition> parts;
    
    if (!data) {
      func(true, parts);
      return;
    }
    
    auto* mbr = reinterpret_cast<MBR::mbr*>(data); //< First sector is the Master Boot Record
    
    for (int i {0}; i < 4; ++i) {
      // all the partitions are offsets to potential Volume Boot Records
      /*
      printf("<P%u> ", i+1);
      printf("Flags: %u\t", mbr->part[i].flags);
      printf("Type: %s\t", MBR::id_to_name( mbr->part[i].type ).c_str() );
      printf("LBA begin: %x\n", mbr->part[i].lba_begin);
      */
      parts.emplace_back(mbr->part[i].flags,     //< flags
                         mbr->part[i].type,      //< id
                         mbr->part[i].lba_begin, //< LBA
                         mbr->part[i].sectors);
    }
    
    func(no_error, parts);
  });
}

template <typename FS>
void Disk<FS>::mount(partition_t part, on_mount_func func) {
  /** For the MBR case, all we need to do is mount on sector 0 */
  if (part == PART_MBR) {
    fs().mount(0, device.size(), func);
  }
  else {
    /**
     *  Otherwise, we will have to read the LBA offset
     *  of the partition to be mounted
     */
    device.read_sector(0,
    [this, part, func] (const void* data)
    {
      if (!data) {
        // TODO: error-case for unable to read MBR
        func(true);
        return;
      }
      
      auto* mbr = reinterpret_cast<MBR::mbr*>(data); //< Treat data as MBR
      
      auto pint = static_cast<int>(part - 1); //< Treat VBR1 as index 0 etc.

      /** Get LBA from selected partition */
      auto lba_base = mbr->part[pint].lba_begin;
      auto lba_size = mbr->part[pint].sectors;
      
      /**
       *  Call the filesystems mount function
       *  with lba_begin as base address
       */
      fs().mount(lba_base, lba_size, func);
    });
  }
}

template <typename FS>
std::string Disk<FS>::Partition::name() const {
  return MBR::id_to_name(id);
}

} //< namespace fs
