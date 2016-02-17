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

#ifndef HW_PCI_DEVICE_HPP
#define HW_PCI_DEVICE_HPP

#include <cstdio>
#include <common>

namespace PCI {

static const uint16_t  CONFIG_ADDR           {0xCF8U};
static const uint16_t  CONFIG_DATA           {0xCFCU};
static const uint8_t   CONFIG_INTR           {0x3CU};

static const uint8_t   CONFIG_VENDOR         {0x00U};
static const uint8_t   CONFIG_CLASS_REV      {0x08U};

static const uint8_t   CONFIG_BASE_ADDR_0    {0x10U};

static const uint32_t  BASE_ADDRESS_MEM_MASK {~0x0FUL};
static const uint32_t  BASE_ADDRESS_IO_MASK  {~0x03UL};

static const uint32_t  WTF                   {0xffffffffU};

/** 
 *  @brief PCI device message format
 *
 *  Used to communicate with PCI devices
 */
union msg {

  //! The whole message
  uint32_t data;
  
  /**
   *  Packed attribtues, ordered low to high.
   *
   *  @note: Doxygen thinks this is a function - it's not
   *
   *  it's a GCC-directive.
   */
  struct __attribute__((packed)) {
    //! The PCI register
    uint8_t reg;
    
    //! The 16-bit PCI-address @see pci_addr()
    uint16_t addr;
    uint8_t  code;
  };
}; //< union msg
  
/** Relevant class codes (many more) */
enum classcode_t {
  OLD,
  STORAGE,
  NIC,
  DISPLAY, // VirtioConsole
  MULTIMEDIA,
  MEMORY,
  BRIDGE,
  OTHER=255
}; //< enum classcode_t
} //< namespace PCI

/**
 *  @brief Communication class for all PCI devices
 *  
 *  All low level communication with PCI devices should (ideally) go here.
 *  
 *  @todo 
 *  - Consider if we ever need to separate the address into 'bus/dev/func' parts.
 *  - Do we ever need anything but PCI Devices?
*/
class PCI_Device { // public Device //Why not? A PCI device is too general to be accessible?
public:
  
  enum {
    VENDOR_AMD     = 0x1022,
    VENDOR_INTEL   = 0x8086,
    VENDOR_CIRRUS  = 0x1013,
    VENDOR_VIRTIO  = 0x1AF4,
    VENDOR_REALTEK = 0x10EC
  };

  /**
   *  Constructor
   *
   *  @param pci_addr:  A 16-bit PCI address.
   *  @param device_id: A device ID, consisting of PCI vendor and product ID's.
   *
   *  @see pci_addr() for more about the address  
   */
  explicit PCI_Device(const uint16_t pci_addr, const uint32_t device_id) noexcept;
  
  //! @brief Read from device with implicit pci_address (e.g. used by Nic)
  uint32_t read_dword(const uint8_t reg) noexcept;

  //! @brief Read from device with explicit pci_addr
  static uint32_t read_dword(const uint16_t pci_addr, const uint8_t reg) noexcept;

  /** 
   *  Probe for a device on the given address
   *
   *  @param pci_addr: the address to probe
   *     
   *  @deprecated We got a 20% performance degradation using this for probing
   *
   *  @see PCI_Device()
   */
  static PCI_Device* Create(uint16_t pci_addr);  

  // @brief Get a device by address. @see pci_addr().
  static PCI_Device* get(uint16_t pci_addr);

  // @brief Get a device by individual address parts.
  // @todo  Will we ever need this?  
  static PCI_Device* get(int busno, int devno, int funcno);
  
  /** A descriptive name  */
  inline const char* name();
  
  /**
   *  Get the PCI address of device.
   *
   *  The address is a composite of 'bus', 'device' and 'function', usually used
   *  (i.e. by Linux) to designate a PCI device.
   *
	 *  @return: The address of the device
   */
  inline uint16_t pci_addr() const noexcept
  { return pci_addr_; };
    
  /** Get the pci class code. */
  inline PCI::classcode_t classcode() const noexcept
  { return static_cast<PCI::classcode_t>(devtype_.classcode); }
  
  inline uint16_t rev_id() const noexcept
  { return devtype_.rev_id; }

  /** Get the pci vendor and product id */
  inline uint16_t vendor_id() const noexcept
  { return device_id_.vendor; }

  inline uint16_t product_id() const noexcept
  { return device_id_.product; }
  
  /**
   *  Parse all Base Address Registers (BAR's)
   *
   *  Used to determine how to communicate with the device.
   *
   *  This function adds resources to the PCI_Device.
   */
  void probe_resources() noexcept;
  
  /** The base address of the (first) I/O resource */
  uint32_t iobase() const noexcept;

private:
  // @brief The 3-part PCI address
  uint16_t pci_addr_;
  
  //@brief The three address parts derived (if needed)      
  uint8_t busno_  {0};
  uint8_t devno_  {0};
  uint8_t funcno_ {0};
  
  // @brief The 2-part ID retrieved from the device
  union vendor_product {
    uint32_t __value;
    struct __attribute__((packed)) {
      uint16_t vendor;
      uint16_t product;
    };
  } device_id_;

  // @brief The class code (device type)
  union class_revision {
    uint32_t reg;
    struct __attribute__((packed)) {
      uint8_t rev_id;
      uint8_t prog_if;
      uint8_t subclass;
      uint8_t classcode;
    };
    struct __attribute__((packed)) {
      uint16_t class_subclass;
      uint8_t __prog_if; //Overlaps the above
      uint8_t revision;        
    };
  } devtype_;

  // @brief Printable names
  const char* classname_;
  const char* vendorname_;
  const char* productname_;
  
  // Device Resources

  //! @brief Resource types, "Memory" or "I/O"
  enum resource_t { RES_MEM, RES_IO };
  
  /** A device resource - possibly a list */
  template<resource_t RT>
  struct Resource {
    const resource_t type {RT};
    uint32_t start_;
    uint32_t len_;
    Resource<RT>* next {nullptr};
    Resource<RT>(const uint32_t start, const uint32_t len) : start_{start}, len_{len} {};
  };

  //! @brief Resource lists. Members added by add_resource();
  Resource<RES_MEM>* res_mem_ {nullptr};
  Resource<RES_IO>*  res_io_  {nullptr};
   

  //! @brief Write to device with implicit pci_address (e.g. used by Nic)
  void write_dword(const uint8_t reg, const uint32_t value) noexcept;

  /**
   *  Add a resource to a resource queue.
   *
   *  (This seems pretty dirty; private class, reference to pointer etc.) */
  template<resource_t R_T>
  void add_resource(Resource<R_T>* res, Resource<R_T>*& Q) noexcept {
    Resource<R_T>* q;
    if (Q) {
      q = Q;
      while (q->next) q = q->next;
      q->next = res;
    } else {
      Q = res;
    }
  }
}; //< class PCI_Device

#endif //< HW_PCI_DEVICE_HPP
