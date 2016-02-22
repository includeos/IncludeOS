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
#ifndef HW_DISK_DEVICE_HPP
#define HW_DISK_DEVICE_HPP

#include <memory>
#include <cstdint>
#include <functional>

namespace hw {

class IDiskDevice {
public:
  using block_t  = uint64_t; //< Disk device block size
  using buffer_t = std::shared_ptr<uint8_t>;
  
  /** Delegate for result of reading a disk sector */
  using on_read_func = std::function<void(buffer_t)>;
  
  /** Human-readable name of this disk controller */
  virtual const char* name() const noexcept = 0;
  
  /**
   *  Read sector(s) from blk and call reader with result
   *  A nullptr is passed to result if something bad happened
   */
  virtual void read_sector(block_t blk, on_read_func reader) = 0;
  virtual void read_sectors(block_t blk, block_t count, on_read_func) = 0;
  
  /** The size of the disk in sectors */
  virtual block_t size() const noexcept = 0;

  /** Default destructor */
  virtual ~IDiskDevice() noexcept = default;
}; //< class IDiskDevice

} //< namespace hw

#endif //< HW_DISK_DEVICE_HPP
