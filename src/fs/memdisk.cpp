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

#define likely(x)       __builtin_expect(!!(x), 1)
#define unlikely(x)     __builtin_expect(!!(x), 0)

extern "C" {
  char _DISK_START_;
  char _DISK_END_;
}

namespace fs {

MemDisk::MemDisk() noexcept
  : image_start { &_DISK_START_ },
    image_end   { &_DISK_END_ }
{}

void MemDisk::read(block_t blk, on_read_func callback) {
  auto* sector_loc = ((char*) image_start) + blk * block_size();
  // Disallow reading memory past disk image
  if (unlikely(sector_loc >= image_end))
  {
    callback(buffer_t()); return;
  }
  
  auto* buffer = new uint8_t[block_size()];
  assert( memcpy(buffer, sector_loc, block_size()) == buffer );
  
  callback( buffer_t(buffer, std::default_delete<uint8_t[]>()) );
}

void MemDisk::read(block_t start, block_t count, on_read_func callback) {
  auto* start_loc = ((char*) image_start) + start * block_size();
  auto* end_loc   = start_loc + count * block_size();
  // Disallow reading memory past disk image
  if (unlikely(end_loc >= image_end))
  {
    callback(buffer_t()); return;
  }
  
  auto* buffer = new uint8_t[count * block_size()];
  assert( memcpy(buffer, start_loc, count * block_size()) == buffer );
  
  callback( buffer_t(buffer, std::default_delete<uint8_t[]>()) );
}

MemDisk::buffer_t MemDisk::read_sync(block_t blk)
{
  auto* loc = ((char*) image_start) + blk * block_size();
  // Disallow reading memory past disk image
  if (unlikely(loc >= image_end))
    return buffer_t();
  
  auto* buffer = new uint8_t[block_size()];
  assert( memcpy(buffer, loc, block_size()) == buffer );
  
  return buffer_t(buffer, std::default_delete<uint8_t[]>());
}

MemDisk::block_t MemDisk::size() const noexcept {
  return ((char*) image_end - (char*) image_start) / SECTOR_SIZE;
}
  
} //< namespace fs
