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
#ifndef HW_DRIVE_HPP
#define HW_DRIVE_HPP

#include <memory>
#include <cstdint>
#include <delegate>

namespace hw
{

  class Drive {
  public:
    using block_t = uint64_t; //< Disk device block size
    using buffer_t = std::shared_ptr<uint8_t>;

    // Delegate for result of reading a disk sector
    using on_read_func = delegate<void(buffer_t)>;

    static const char* device_type()
    { return "Drive"; }

    virtual std::string blkname() const = 0;

    /** Human-readable name of this disk controller  */
    virtual const char* name() const noexcept = 0;

    /** The size of the disk in whole sectors */
    virtual block_t size() const noexcept = 0;

    /** Returns the optimal block size for this device */
    virtual block_t block_size() const noexcept = 0;

    /**
     *  Read block(s) from blk and call func with result
     *  A null-pointer is passed to result if something bad happened
     *  Validate using !buffer_t:
     *  if (!buffer)
     *     error("Device failed to read sector");
     **/
    virtual void read(block_t blk, on_read_func func) = 0;
    virtual void read(block_t blk, size_t count, on_read_func) = 0;

    /** read synchronously the block @blk  */
    virtual buffer_t read_sync(block_t blk) = 0;
    virtual buffer_t read_sync(block_t blk, size_t count) = 0;

    virtual void deactivate() = 0;

    virtual ~Drive() noexcept = default;
    
  protected:
    Drive();
    int blkid;
  }; //< class Drive

} //< namespace hw

#endif //< HW_DRIVE_HPP
