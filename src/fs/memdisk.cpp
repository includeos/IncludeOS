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

#include <cstring>
#include <cassert>
#include <cstdio>

#include <fs/memdisk.hpp>

extern "C" {
  char _DISK_START_;
  char _DISK_END_;
}

namespace fs {

MemDisk::MemDisk() noexcept
  : image_start { &_DISK_START_ },
    image_end   { &_DISK_END_ }
{}

void MemDisk::read_sector(block_t blk, on_read_func reader) {
  auto* sector_loc = ((char*) image_start) + blk * SECTOR_SIZE;
  assert(sector_loc < image_end); //< Disallow reading memory past disk image
  
  
  auto* buffer = new uint8_t[SECTOR_SIZE]; //< Copy block to new memory
  assert( memcpy(buffer, sector_loc, SECTOR_SIZE) == buffer );
  
  reader(buffer);
}

void MemDisk::read_sectors(block_t start, block_t count, on_read_func reader) {
  auto* start_loc = ((char*) image_start) + start * SECTOR_SIZE;
  auto* end_loc   = start_loc + count * SECTOR_SIZE;
  
  assert(end_loc < image_end); //< Disallow reading memory past disk image
  
  auto* buffer = new uint8_t[count * SECTOR_SIZE]; //< Copy block to new memory
  assert( memcpy(buffer, start_loc, count * SECTOR_SIZE) == buffer );
  
  reader(buffer);
}

uint64_t MemDisk::size() const noexcept {
  return ((char*) image_end - (char*) image_start) / SECTOR_SIZE;
}
  
} //< namespace fs
