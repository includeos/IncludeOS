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

#include <delegate>
#include <list>

#include "disk.hpp"
#include "pci_device.hpp"

namespace hw {

  /** IDE device driver  */
  class IDE : public Drive {
  public:
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
    virtual const char* name() const noexcept override
    { return "IDE Controller"; }

    /** Returns the optimal block size for this device.  */
    virtual block_t block_size() const noexcept override
    { return 512; }

    virtual void read(block_t blk, on_read_func reader) override;
    virtual void read(block_t blk, size_t cnt, on_read_func cb) override;

    /** read synchronously from IDE disk  */
    virtual buffer_t read_sync(block_t blk) override;
    virtual buffer_t read_sync(block_t blk, size_t cnt) override;

    virtual block_t size() const noexcept override
    { return _nb_blk; }

    static void set_irq_mode(const bool on) noexcept;

    static void wait_status_busy() noexcept;
    static void wait_status_flags(const int flags, const bool set) noexcept;

  private:
    void set_drive(const uint8_t drive) const noexcept;
    void set_nbsectors(const uint8_t cnt) const noexcept;
    void set_blocknum(block_t blk) const noexcept;
    void set_command(const uint16_t command) const noexcept;

    void callback_wrapper();
    void enable_irq_handler();

  private:
    hw::PCI_Device& _pcidev; // PCI device
    uint8_t         _drive;  // Drive id (IDE_MASTER or IDE_SLAVE)
    uint32_t        _iobase; // PCI device io base address
    block_t         _nb_blk; // Max nb blocks of the device
  }; //< class IDE

} //< namespace hw

#endif //< HW_IDE_HPP
