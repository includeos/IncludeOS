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
#ifndef SOLO5_BLOCK_HPP
#define SOLO5_BLOCK_HPP

#include <common>
#include <hw/block_device.hpp>
#include <hw/pci_device.hpp>
#include <virtio/virtio.hpp>
#include <deque>

class Solo5Blk : public hw::Block_device
{
public:

  static std::unique_ptr<Block_device> new_instance()
  {
    return std::make_unique<Solo5Blk>();
  }

  static constexpr size_t SECTOR_SIZE = 512;

  std::string device_name() const override { // stays
    return "vblk" + std::to_string(id());
  }

  /** Human readable name. */
  const char* driver_name() const noexcept override { // stays
    return "Solo5Blk";
  }

  // returns the optimal block size for this device
  block_t block_size() const noexcept override { // stays
    return SECTOR_SIZE; // some multiple of sector size
  }

  void read(block_t blk, size_t cnt, on_read_func reader) override {
    // solo5 doesn't support async blk IO at the moment
    reader( read_sync(blk, cnt) );
  }

  buffer_t read_sync(block_t, size_t) override;

  block_t size() const noexcept override;

  void deactivate() override; // stays

  /** Constructor. @param pcidev an initialized PCI device. */
  Solo5Blk();

private:
};

#endif
