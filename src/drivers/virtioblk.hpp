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
#ifndef VIRTIO_BLOCK_HPP
#define VIRTIO_BLOCK_HPP

#include <common>
#include <hw/block_device.hpp>
#include <hw/pci_device.hpp>
#include <virtio/virtio.hpp>
#include <deque>

/** Virtio-net device driver.  */
class VirtioBlk : public Virtio, public hw::Block_device
{
public:

  static std::unique_ptr<Block_device> new_instance(hw::PCI_Device& d)
  { return std::make_unique<VirtioBlk>(d); }

  static constexpr size_t SECTOR_SIZE = 512;

  std::string device_name() const override {
    return "vblk" + std::to_string(id());
  }

  /** Human readable name. */
  const char* driver_name() const noexcept override {
    return "VirtioBlk";
  }

  // returns the optimal block size for this device
  block_t block_size() const noexcept override {
    return SECTOR_SIZE; // some multiple of sector size
  }

  block_t size() const noexcept override {
    return config.capacity;
  }

  // read @blk + @cnt from disk, call func with buffer when done
  void read(block_t blk, size_t cnt, on_read_func cb) override;

  // unsupported sync reads
  buffer_t read_sync(block_t, size_t) override {
    return buffer_t();
  }

  void deactivate() override;

  /** Constructor. @param pcidev an initialized PCI device. */
  VirtioBlk(hw::PCI_Device& pcidev);

private:
  struct virtio_blk_geometry_t
  {
    uint16_t cyls;
    uint8_t  heds;
    uint8_t  sect;
  } __attribute__((packed));

  struct virtio_blk_config_t
  {
    uint64_t capacity;
    uint32_t size_max;
    uint32_t seg_max;
    virtio_blk_geometry_t geometry;
    uint32_t blk_size;           // Block size of device
    uint8_t physical_block_exp;  // Exponent for physical block per logical block
    uint8_t alignment_offset;    // Alignment offset in logical blocks
    uint16_t min_io_size;        // Minimum I/O size without performance penalty in logical blocks
    uint32_t opt_io_size;        // Optimal sustained I/O size in logical blocks
  };

  struct scsi_header_t
  {
    uint32_t type;
    uint32_t ioprio;
    uint64_t sector;
  };
  struct blk_io_t
  {
    uint8_t      sector[512];
  };
  typedef delegate<void(uint8_t*)> request_handler_t;
  struct blk_resp_t
  {
    uint8_t           status;
    request_handler_t handler;
  };

  struct request_t
  {
    scsi_header_t hdr;
    blk_io_t      io;
    blk_resp_t    resp;

    request_t(uint64_t blk, request_handler_t cb);
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

  void msix_conf_handler();

  // need at least 3 tokens free to ship a request
  inline bool free_space() const noexcept
  { return req.num_free() >= 3; }

  // need many free tokens free to efficiently ship requests
  inline bool lots_free_space() const noexcept
  { return req.num_free() >= 32; }

  // add one request to queue and kick
  void shipit(request_t*);

  void handle(request_t*);

  Virtio::Queue req;

  // configuration as read from paravirtual PCI device
  virtio_blk_config_t config;

  // queue waiting for space in vring
  std::deque<request_t*> jobs;
  size_t    inflight;

  // stack of dequeued requests to be processed
  std::vector<request_t*> received;

  // stat counters
  uint32_t* errors;
  uint32_t* requests;
};

#endif
