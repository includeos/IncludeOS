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

#include <cstdint>
#include <common>
#include <vector>
#include <unordered_map>

#define  PCI_CAP_ID_AF        0x13	/* PCI Advanced Features */
#define  PCI_CAP_ID_MAX       PCI_CAP_ID_AF
#define  PCI_EXT_CAP_ID_PASID 0x1B	/* Process Address Space ID */
#define  PCI_EXT_CAP_ID_MAX   PCI_EXT_CAP_ID_PASID

namespace PCI {

  static const uint16_t  CONFIG_ADDR           {0xCF8U};
  static const uint16_t  CONFIG_DATA           {0xCFCU};
  static const uint8_t   CONFIG_INTR           {0x3CU};

  static const uint8_t   CONFIG_VENDOR         {0x00U};
  static const uint8_t   CONFIG_CLASS_REV      {0x08U};

  static const uint8_t   CONFIG_BASE_ADDR_0    {0x10U};

  static const uint32_t  BASE_ADDRESS_MEM_MASK {~0x0FU};
  static const uint32_t  BASE_ADDRESS_IO_MASK  {~0x03U};

  static const uint32_t  WTF                   {~0x0U};

  static const uint32_t  SOLO5_NET_DUMMY_ADDR  {0xFFFE};
  static const uint32_t  SOLO5_BLK_DUMMY_ADDR  {0xFFFF};

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
  enum classcode : uint8_t {
    OLD = 0,
    STORAGE,
    NIC,
    DISPLAY,
    MULTIMEDIA,
    MEMORY,
    BRIDGE,
    COMMUNICATION,
    BASE_SYSTEM_PER,
    INPUT_DEVICE,
    DOCKING_STATION,
    PROCESSOR,
    SERIAL_BUS,
    WIRELESS,
    IO_CTL,
    SATELLITE,
    ENCRYPTION,
    SIGPRO,
    OTHER=255


  }; //< enum classcode_t

  enum vendor_t : uint16_t {
    VENDOR_AMD     = 0x1022,
    VENDOR_INTEL   = 0x8086,
    VENDOR_CIRRUS  = 0x1013,
    VENDOR_VIRTIO  = 0x1AF4,
    VENDOR_REALTEK = 0x10EC,
    VENDOR_VMWARE  = 0x15AD,
    VENDOR_SOLO5   = 0x5050,
    VENDOR_QEMU    = 0x1B36
  };

  static inline const char* classcode_str(uint8_t code);
  static inline const char* vendor_str(uint16_t code);

  struct Resource {
    uintptr_t  start;
    size_t     len;
  };

  static const uint8_t   RES_IO  = 0;
  static const uint8_t   RES_MEM = 1;

} //< namespace PCI

namespace hw {


struct msix_t;
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

    /**
     *  Constructor
     *
     *  @param pci_addr:  A 16-bit PCI address.
     *  @param device_id: A device ID, consisting of PCI vendor and product ID's.
     *
     *  @see pci_addr() for more about the address
     */
    explicit PCI_Device(const uint16_t pci_addr, const uint32_t, const uint32_t);

    //! @brief Read from device with implicit pci_address (e.g. used by Nic)
    uint32_t read32(const uint8_t reg) noexcept;

    //! @brief Read from device with explicit pci_addr
    static uint32_t read_dword(const uint16_t pci_addr, const uint8_t reg) noexcept;

    //! @brief Write to device with implicit pci_address (e.g. used by Nic)
    void write_dword(const uint8_t reg, const uint32_t value) noexcept;

    uint16_t read16(const uint8_t reg) noexcept;
    void write16(const uint8_t reg, const uint16_t value) noexcept;

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
    uint16_t pci_addr() const noexcept
    { return pci_addr_; };

    /** Get the pci class code. */
    uint8_t classcode() const noexcept
    { return devtype_.classcode; }

    uint8_t subclass() const noexcept
    { return devtype_.subclass; }

    uint16_t rev_id() const noexcept
    { return devtype_.rev_id; }

    /** Get the pci vendor and product id */
    uint16_t vendor_id() const noexcept
    { return device_id_.vendor; }

    uint16_t product_id() const noexcept
    { return device_id_.product; }

    uint32_t vendor_product() const noexcept
    { return device_id_.both; }

    /**
     *  Parse all Base Address Registers (BAR's)
     *
     *  Used to determine how to communicate with the device.
     *
     *  This function adds resources to the PCI_Device.
     */
    void probe_resources() noexcept;

    /** The base address of the I/O resource */
    uint32_t iobase() const {
      return m_resources.at(this->m_iobase).start;
    }

    typedef uint32_t pcicap_t;
    void parse_capabilities();

    void deactivate();

    // enable INTX (in case it was disabled)
    void intx_enable();
    // returns true if interrupt is asserted
    bool intx_status();

    // return max number of possible MSI-x vectors for this device
    // or, zero if MSI-x support is not enabled
    uint8_t get_msix_vectors();
    // setup one msix vector directed to @cpu on @irq
    int setup_msix_vector(uint8_t cpu, uint8_t irq);
    // redirect MSI-X vector to another CPU
    void rebalance_msix_vector(uint16_t index, uint8_t cpu, uint8_t irq);
    // true if msix is enabled
    bool has_msix() const noexcept {
      return this->msix != nullptr;
    }
    // deactivate msix (mask off vectors)
    void deactivate_msix();
    // MSI and MSI-X capabilities for this device
    // the cap offsets and can also be used as boolean to determine
    // device MSI/MSIX support
    int msi_cap();
    int msix_cap();
    // enable msix (and disable intx)
    void init_msix();

    // resource handling
    const PCI::Resource& get_bar(uint8_t id) const
    {
      return m_resources.at(id);
    }
    bool validate_bar(uint8_t id) const noexcept
    {
      return id < m_resources.size();
    }
    void* allocate_bar(uint8_t id, int pages);

    // @brief The 2-part ID retrieved from the device
    union vendor_product_t {
      uint32_t both;
      struct __attribute__((packed)) {
        uint16_t vendor;
        uint16_t product;
      };
    };
    // @brief The class code (device type)
    union class_revision_t {
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
    };

    std::string to_string() const;

  private:
    // @brief The 3-part PCI address
    uint16_t pci_addr_;

    vendor_product_t device_id_;
    class_revision_t devtype_;

    //! @brief List of PCI BARs
    int m_iobase = -1;
    std::array<PCI::Resource, 6> m_resources;

    std::array<pcicap_t, PCI_CAP_ID_MAX+1> caps;

    // has msix support if not null
    msix_t*  msix = nullptr;
  }; //< class PCI_Device

} //< namespace hw

static const char* PCI::classcode_str(uint8_t code){
  const std::unordered_map<uint8_t, const char*> classcodes {
      {classcode::OLD, "Old"},
      {classcode::STORAGE, "Storage controller"},
      {classcode::NIC, "Network controller"},
      {classcode::DISPLAY, "Display controller"},
      {classcode::MULTIMEDIA, "Multimedia device"},
      {classcode::MEMORY, "Memory controller"},
      {classcode::BRIDGE, "Bridge device"},
      {classcode::COMMUNICATION, "Simple comm. controller "},
      {classcode::BASE_SYSTEM_PER,"Base system periph."},
      {classcode::INPUT_DEVICE, "Input device"},
      {classcode::DOCKING_STATION, "Docking station"},
      {classcode::PROCESSOR, "Processor"},
      {classcode::SERIAL_BUS, "Serial bus controller"},
      {classcode::WIRELESS, "Wireless"},
      {classcode::IO_CTL, "Intelligent IO controller"},
      {classcode::SATELLITE, "Satellite comm. controller"},
      {classcode::ENCRYPTION, "Encryption / decryption controller"},
      {classcode::SIGPRO,"Sigpro"},
      {classcode::OTHER, "Other"}
    };

  auto it = classcodes.find(code);
  if (it != classcodes.end())
    return it->second;

  return "Unknown classcode";
}

static const char* PCI::vendor_str(uint16_t code){
  const std::unordered_map<uint16_t, const char*> classcodes {
    {VENDOR_AMD,     "AMD"},
    {VENDOR_INTEL,   "Intel"},
    {VENDOR_CIRRUS,  "Cirrus"},
    {VENDOR_VIRTIO,  "VirtIO"} ,
    {VENDOR_REALTEK, "REALTEK"},
    {VENDOR_VMWARE,  "VMWare"},
    {VENDOR_QEMU,    "QEMU"}
  };

  auto it = classcodes.find(code);
  return it == classcodes.end() ? "Unknown vendor" : it->second;
}

#endif //< HW_PCI_DEVICE_HPP
