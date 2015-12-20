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

#ifndef VIRTIO_VIRTIOBLK_HPP
#define VIRTIO_VIRTIOBLK_HPP

#include <common>
#include "../hw/pci_device.hpp"
#include "virtio.hpp"
#include <delegate>

/** Virtio-net device driver.  */
class VirtioBlk : Virtio
{
public:
  typedef uint64_t block_t;
  typedef delegate<void(int, block_t, const char*)> on_read_func;
  static const int SECTOR_SIZE = 512;
  
  /** Human readable name. */
  const char* name();  
  
  // returns the optimal block size for this device
  constexpr block_t block_size() const
  {
    return SECTOR_SIZE; // some multiple of sector size
  }
  
  void read (block_t blk, on_read_func func);
  void write(block_t blk, const char* data);
  
  /** Constructor. @param pcidev an initialized PCI device. */
  VirtioBlk(PCI_Device& pcidev);
  
private:
  Virtio::Queue req;
  
  struct virtio_blk_geometry_t
  {
    uint16_t cyls;
    uint8_t  heds;
    uint8_t  sect;
  };
  
  struct virtio_blk_config_t
  {
    uint64_t capacity;
    /*
    uint32_t size_max;
    uint32_t seg_max;
    virtio_blk_geometry_t geometry;
    uint32_t blk_size; */
  };
  
  struct virtio_blk_request_t
  {
    uint32_t type;
    uint32_t ioprio;
    uint64_t sector;
    char     data[SECTOR_SIZE];
    uint8_t  status;
  };
  
  /** Get virtio PCI config. @see Virtio::get_config.*/
  void get_config();
  
  /** Service the RX Queue. 
      Push incoming data up to linklayer, dequeue RX buffers. */
  void service_RX();
  
  /** Service the TX Queue 
      Dequeue used TX buffers. @note: This function does not take any 
      responsibility for memory management. */
  void service_TX();
  
  /** Handle device IRQ. 
      
      Will look for config. changes and service RX/TX queues as necessary.*/
  void irq_handler();
  
  /** Allocate and queue buffer from bufstore_ in RX queue. */
  int add_receive_buffer();  
  
  // configuration as read from paravirtual PCI device
  virtio_blk_config_t config;
};

#endif
