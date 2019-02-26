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
#ifndef FS_MEMDISK_HPP
#define FS_MEMDISK_HPP

#include <cstdint>
#include <hw/block_device.hpp>

namespace fs {

  class MemDisk : public hw::Block_device {
  public:
    static constexpr size_t SECTOR_SIZE = 512;

    static MemDisk& get() noexcept {
      static MemDisk memdisk;
      return memdisk;
    }

    std::string device_name() const override {
      return "memdisk" + std::to_string(id());
    }

    virtual const char* driver_name() const noexcept override
    { return "MemDisk"; }

    virtual block_t size() const noexcept override;

    /** Returns the optimal block size for this device.  */
    virtual block_t block_size() const noexcept override
    { return SECTOR_SIZE; }

    void read(block_t blk, size_t cnt, on_read_func reader) override {
      reader( read_sync(blk, cnt) );
    }

    buffer_t read_sync(block_t blk, size_t cnt) override;

    explicit MemDisk() noexcept;
    explicit MemDisk(const char* start, const char* end) noexcept;

    void deactivate() override;

  private:
    const char* image_start_;
    const char* image_end_;

    uint64_t& stat_read;
  }; //< class MemDisk

} //< namespace fs

#endif //< FS_MEMDISK_HPP
