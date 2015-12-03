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

/** @Todo : Implement */
class Block { };

/** @Todo : Implement */
class DiskError;

/** A public interface for block devices
    
    @Todo: This is just a stub. 
    Currently (v0.6.3-proto) only the bootloader access disks.
    
    The requirements for a driver is implicitly given by how it's used below,
    rather than explicitly by proper inheritance.  */

template <typename DRIVER>
class Disk{ 
  
public:

  typedef DRIVER driver; 
  using Blocknr = uint32_t;
 
  
  using On_read = delegate<void(Block& b)>;
  using On_error = delegate<void(DiskError)>;
  
  /** Get a readable name. */
  inline const char* name() { return driver_.name(); }
  
  inline uint32_t block_size () const 
  { return driver_.block_size(); }    
  
  inline void read_block(Blocknr nr, On_read onread, On_error onerr)
  { return driver_.read_block(nr, onread, onerr); }
  
  inline void write_block(Blocknr nr, Block blk, On_error onerr)
  { return driver_.write_block(nr, blk, onerr); }
  
private:
  
  DRIVER driver_;
  
  /** Constructor. 
      
      Just a wrapper around the driver constructor.
      @note The Dev-class is a friend and will call this */
  Disk(PCI_Device& d): driver_(d) {}
  
  friend class Dev;

};

/** @todo : At least implement virtio block devices. */
class Virtio_block{
public: 
  const char* name(){ return "E1000 Driver"; }
  //...whatever the Nic class implicitly needs
  
};

/** Hopefully somebody will port a driver for this one */
class IDE;

#endif
