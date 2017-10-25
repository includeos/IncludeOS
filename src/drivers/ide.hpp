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

#ifndef HW_IDE_HPP
#define HW_IDE_HPP

#include <hw/block_device.hpp>
#include <array>
#include <delegate>
#include <deque>
#include <memory>
#include <string>

namespace hw {
  class PCI_Device;
}

/** IDE device driver  */
class IDE : public hw::Block_device {
public:
  static const int SECTOR_SIZE  = 512;
  static const int SECTOR_ARRAY = 256;
  enum selector_t
  {
    MASTER = 0x00,
    SLAVE = 0x10
  };

  /**
   * Constructor
   *
   * @param pcidev: An initialized PCI device
   */
  explicit IDE(hw::PCI_Device& pcidev, selector_t);

  /** Human-readable name of this disk controller  */
  virtual const char* driver_name() const noexcept override
  { return "IDE Controller"; }

  std::string device_name() const override {
    return "ide" + std::to_string(id());
  }

  /** Returns the optimal block size for this device.  */
  virtual block_t block_size() const noexcept override
  { return SECTOR_SIZE; }

  virtual void read(block_t blk, on_read_func reader) override;
  virtual void read(block_t blk, size_t cnt, on_read_func cb) override;

  /** read synchronously from IDE disk  */
  virtual buffer_t read_sync(block_t blk) override;
  virtual buffer_t read_sync(block_t blk, size_t cnt) override;

  virtual block_t size() const noexcept override
  { return this->num_blocks; }

  void deactivate() override;

  static std::unique_ptr<Block_device> new_instance(hw::PCI_Device& d)
  {
    static bool not_first = false;
    if (not_first == false) {
      not_first = true;
      return std::make_unique<IDE>(d, MASTER);
    }
    return std::make_unique<IDE>(d, SLAVE);
  }
private:
  static void set_irq_mode(bool on) noexcept;
  static void spinwait_status_busy() noexcept;
  static void wait_status_flags(int flags, bool set) noexcept;

  static void set_drive(const uint8_t drive) noexcept;
  static void set_nbsectors(const uint8_t cnt) noexcept;
  static void set_blocknum(block_t blk) noexcept;
  static void set_command(const uint16_t command) noexcept;

  typedef std::array<uint16_t, SECTOR_ARRAY> ide_read_array_t;
  uint8_t         drive_id;
  uint32_t        pci_iobase = 0;
  block_t         num_blocks = 0;

  struct readq_item {
    readq_item(uint8_t id, block_t blk, uint32_t cnt, on_read_func call)
      : drive_id(id), sector(blk), total(cnt), callback(std::move(call))
    {
      buffer = std::make_shared<std::vector<uint8_t>> (total * IDE::SECTOR_SIZE);
    }

    uint8_t* current() {
      return &buffer->at(position * IDE::SECTOR_SIZE);
    }
    bool done() const noexcept { return position == total; }

    uint8_t      drive_id;
    block_t      sector;
    uint32_t     position = 0;
    uint32_t     total;
    buffer_t     buffer;
    on_read_func callback;
  };
  static std::deque<readq_item> read_queue;

  static void begin_reading();
  static void irq_handler();
}; //< class IDE

#endif //< HW_IDE_HPP
