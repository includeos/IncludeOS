// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015-2017 Oslo and Akershus University College of Applied Sciences
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
#ifndef HW_BLOCK_DEVICE_HPP
#define HW_BLOCK_DEVICE_HPP

#include <cstdint>
#include <delegate>
#include <memory>
#include <pmr>
#include <vector>
#include "device.hpp"

namespace hw {

/**
 * This class is an abstract interface for block devices
 */
class Block_device : public Device {
public:
  using block_t       = uint64_t;
  using buffer_t      = os::mem::buf_ptr;
  using on_read_func  = delegate<void(buffer_t)>;
  using on_write_func = delegate<void(bool error)>;

  /**
   * Method to get the type of device
   *
   * @return The type of device as a C-String
   */
  Device::Type device_type() const noexcept override
  { return Device::Type::Block; }

  /**
   * Method to get the name of the device
   *
   * @return The name of the device as a std::string
   */
  virtual std::string device_name() const override = 0;

  /**
   * Method to get the device's identifier
   *
   * @return The device's identifier
   */
  int id() const noexcept
  { return id_; }

  /**
   * Get the human-readable name of this device's controller
   *
   * @return The human-readable name of this device's controller
   */
  virtual const char* driver_name() const noexcept = 0;

  /**
   * Get the size of the device as total number of blocks
   *
   * @return The size of the device as total number of blocks
   */
  virtual block_t size() const noexcept = 0;

  /**
   * Get the optimal block size for this device
   *
   * @return The optimal block size for this device
   */
  virtual block_t block_size() const noexcept = 0;

  /**
   * Read a block of data asynchronously from the device
   *
   * @param blk
   *   The block of data to read from the device
   *
   * @param reader
   *   An operation to perform asynchronously
   *
   * @note A nullptr is passed to the reader if an error occurred
   * @note Validate the reader's input
   *
   * @example
   *   if (buffer == nullptr) {
   *     error("Device failed to read sector");
   *   }
   */
  virtual void read(block_t blk, on_read_func reader) {
    read(blk, 1, std::move(reader));
  }

  /**
   * Read blocks of data asynchronously from the device
   *
   * @param blk
   *   The starting block of data to read from the device
   *
   * @param count
   *   The number of blocks to read from the device
   *
   * @param reader
   *   An operation to perform asynchronously
   *
   * @note A nullptr is passed to the reader if an error occurred
   * @note Validate the reader's input
   *
   * @example
   *   if (buffer == nullptr) {
   *     error("Device failed to read sector");
   *   }
   */
  virtual void read(block_t blk, size_t count, on_read_func reader) = 0;

  /**
   * Read blocks of data synchronously from the device
   *
   * @param blk
   *   The starting block of data to read from the device
   *
   * @param count
   *   The number of blocks to read from the device
   *
   * @return A buffer containing the data or nullptr if an error occurred
   */
  virtual buffer_t read_sync(block_t blk, size_t count=1) = 0;

  /**
   * Method to deactivate the block device
   */
  virtual void deactivate() override = 0;

  virtual ~Block_device() noexcept = default;
protected:
  Block_device() noexcept
  {
    static int counter = 0;
    id_ = counter++;
  }
private:
  int id_;
}; //< class Block_device

} //< namespace hw

#endif //< HW_BLOCK_DEVICE_HPP
