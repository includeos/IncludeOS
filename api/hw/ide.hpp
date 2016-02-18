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
#include <hw/pci_device.hpp>

namespace hw {

  /** IDE device driver.  */
  class IDE
  {
    public:
      typedef uint64_t block_t;
      typedef delegate<void(block_t, char*)> on_read_func;
      typedef delegate<void(block_t, block_t)> on_write_func;

      /** Human readable name.  */
      constexpr const char* name() const
      {
        return "IDE Controller";
      }

      /** Returns the optimal block size for this device.  */
      constexpr block_t block_size() const
      {
        return 512;
      }

      void read(block_t blk, on_read_func del);
      void write(block_t blk, const char* data);

      /**
       * Constructor.
       * @param pcidev An initialized PCI device.
       */
      IDE(hw::PCI_Device& pcidev);

    private:
      void set_drive(uint8_t drive);
      void set_nbsectors(uint8_t cnt);
      void set_blocknum(block_t blk);
      void set_command(uint16_t command);

      void wait_status_busy(void);
      void wait_status_flags(int flags, bool set);

    private:
      hw::PCI_Device& _pcidev; // PCI device
      uint8_t _drive; // Drive id (IDE_MASTER or IDE_SLAVE)
      uint32_t _iobase; // PCI device io base address.
      block_t _nb_blk; // Max nb blocks of the device
  };

} //< namespace hw

#endif //< HW_IDE_HPP
