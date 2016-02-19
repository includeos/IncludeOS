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

#ifndef HW_DISK_HPP
#define HW_DISK_HPP

#include "pci_device.hpp"
#include "disk_device.hpp"

namespace hw {

template <typename DRIVER>
class Disk : public IDiskDevice {
public:
  /** Human readable name.  */
  const char* name() const noexcept override
  {
    return driver.name();
  }
  
  virtual void
  read_sector(block_t blk, on_read_func del) override
  {
    driver.read_sector(blk, del);
  }
  virtual void
  read_sectors(block_t blk, block_t count, on_read_func del) override
  {
    driver.read_sectors(blk, count, del);
  }
  
  virtual block_t size() const noexcept override
  {
    return driver.size();
  }
  
  virtual ~Disk() = default;
  
private:
  DRIVER driver;
  
  /**
   *  Just a wrapper around the driver constructor
   *  @note The Dev-class is a friend and will call this
   */
  explicit Disk(PCI_Device& d): driver{d} {}
  
  friend class Dev;
}; //< class Disk

} //< namespace hw

#endif //< HW_DISK_HPP
