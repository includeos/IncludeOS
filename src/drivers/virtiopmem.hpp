#pragma once
#ifndef VIRTIO_PMEM_HPP
#define VIRTIO_PMEM_HPP

#include <cstdint>
#include <hw/pci_device.hpp>
#include <hw/dax_device.hpp>
#include <modern_virtio/control_plane.hpp>
#include <modern_virtio/split_queue.hpp>

typedef struct __attribute__((packed)) { 
  uint64_t start; 
  uint64_t size; 
} virtio_pmem_config; 

#define VIRTIO_PMEM_REQ_TYPE_FLUSH 0
typedef struct __attribute__((packed)) { 
  uint32_t type; 
} virtio_pmem_req;

typedef struct __attribute__((packed)) { 
  uint32_t ret; 
} virtio_pmem_res;

class VirtioPMEM_device : 
  public Virtio_control, 
  public hw::DAX_device
{
public:
  /** Constructor and VirtioFS driver factory */
  VirtioPMEM_device(hw::PCI_Device& d);

  static std::unique_ptr<hw::DAX_device> new_instance(hw::PCI_Device& d);

  int id() const noexcept override;

  /** Overriden device base functions */
  std::string device_name() const override;

  /** DAX region */
  void *start_addr() const noexcept override {
    return reinterpret_cast<void*>(_config->start);
  }

  uint64_t size() const noexcept override {
    return reinterpret_cast<uint64_t>(_config->size);
  }
  
  /** Other */
  void flush() override;
  void deactivate() override;

private:
  Split_queue _req;
  int _id;
  virtio_pmem_config *_config;
};


#endif // VIRTIO_PMEM_HPP