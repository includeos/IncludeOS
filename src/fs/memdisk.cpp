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

#include <cstdio>
#include <cstring>

#include <common>
#include <fs/memdisk.hpp>
#include <statman>

extern "C" {
  char _DISK_START_;
  char _DISK_END_;
}

namespace fs {

  MemDisk::MemDisk() noexcept
  : Drive(),
    image_start_ { &_DISK_START_ },
    image_end_   { &_DISK_END_ },
    
    stat_read( Statman::get().create(
               Stat::UINT64, blkname() + ".reads").get_uint64() )
  {  }

  MemDisk::buffer_t MemDisk::read_sync(block_t blk)
  {
    stat_read++;
    
    auto sector_loc = image_start_ + blk * block_size();
    // Disallow reading memory past disk image
    if (UNLIKELY(sector_loc >= image_end_))
      return buffer_t{};

    auto buffer = new uint8_t[block_size()];
    memcpy(buffer, sector_loc, block_size());

    return buffer_t{buffer, std::default_delete<uint8_t[]>()};
  }
  
  MemDisk::buffer_t MemDisk::read_sync(block_t blk, size_t cnt)
  {
    stat_read++;
    
    auto start_loc = image_start_ + blk * block_size();
    auto end_loc = start_loc + cnt * block_size();
    
    // Disallow reading memory past disk image
    if (UNLIKELY(end_loc >= image_end_))
      return buffer_t{};

    auto buffer = new uint8_t[cnt * block_size()];
    memcpy(buffer, start_loc, cnt * block_size());

    return buffer_t{buffer, std::default_delete<uint8_t[]>()};
  }

  MemDisk::block_t MemDisk::size() const noexcept {
    // we are NOT going to round up to "support" unevenly sized
    // disks that are not created as multiples of sectors
    return (image_end_ - image_start_) / block_size();
  }

  void MemDisk::deactivate() {
    
  }

} //< namespace fs
