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

template <typename DRIVER>
class Disk{ 
  
public:
  typedef DRIVER  driver; 
  typedef typename driver::block_t       block_t;
  typedef typename driver::on_read_func  on_read_func;
  typedef typename driver::on_write_func on_write_func;
  
  /** Get a readable name. */
  inline const char* name() { return driver_.name(); }
  
  inline block_t block_size() const 
  {
    return driver_.block_size();
  }
  
  inline void read_block(block_t blk, on_read_func on_read)
  {
    driver_.read(blk, on_read);
  }
  
  inline void write_block(block_t, const char*, on_write_func)
  {
    //return driver_.write(blk, data, on_write);
  }
  
private:
  DRIVER driver_;
  
  /** Constructor. 
      
      Just a wrapper around the driver constructor.
      @note The Dev-class is a friend and will call this */
  Disk(PCI_Device& d): driver_(d) {}
  
  friend class Dev;
};

/** Hopefully somebody will port a driver for this one */
class IDE;

#endif
