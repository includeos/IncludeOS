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

#include <assert.h>
#include <info>
#include <hw/pci.hpp>
#include <hw/pci_device.hpp>
#include <hw/msi.hpp>

namespace hw {

  static constexpr std::array<const char*,3> bridge_subclasses {
    "Host",
    "ISA",
    "Other"
  };

  static constexpr std::array<const char*,2> nic_subclasses {
    "Ethernet",
    "Other"
  };

  static unsigned long pci_size(const unsigned long base, const unsigned long mask) noexcept {
    // Find the significant bits
    unsigned long size = mask & base;

    // Get the lowest of them to find the decode size
    size &= ~(size - 1);

    return size;
  }

  void PCI_Device::probe_resources() noexcept
  {
    //Find resources on this PCI device (scan the BAR's)
    for (int bar = 0; bar < 6; ++bar)
    {
      //Read the current BAR register
      uint32_t reg = PCI::CONFIG_BASE_ADDR_0 + (bar << 2);
      uint32_t value = read32(reg);

      if (!value) continue;

      //Write all 1's to the register, to get the length value (osdev)
      write_dword(reg, 0xFFFFFFFF);
      uint32_t len = read32(reg);

      //Put the value back
      write_dword(reg, value);

      uint32_t unmasked_val  {0};
      uint32_t pci__size     {0};

      if (value & 1) {
        // Resource type IO
        unmasked_val = value & PCI::BASE_ADDRESS_IO_MASK;
        pci__size = pci_size(len, PCI::BASE_ADDRESS_IO_MASK & 0xFFFF);
        this->m_iobase = bar;

      } else {
        // Resource type Mem
        unmasked_val = value & PCI::BASE_ADDRESS_MEM_MASK;
        pci__size = pci_size(len, PCI::BASE_ADDRESS_MEM_MASK);
      }
      this->m_resources.at(bar) = {unmasked_val, pci__size};

      INFO2("|  |- BAR%d %s @ 0x%x, size %i ", bar,
            (value & 1 ? "I/O" : "Mem"), unmasked_val, pci__size);
    }

  }

  PCI_Device::PCI_Device(const uint16_t pci_addr,
                         const uint32_t device_id,
                         const uint32_t devclass)
      : pci_addr_{pci_addr}, device_id_{device_id}
  {
    // set master, mem and io flags
    uint32_t cmd = read32(PCI_CMD_REG);
    cmd |= PCI_COMMAND_MASTER | PCI_COMMAND_MEM | PCI_COMMAND_IO;
    write_dword(PCI_CMD_REG, cmd);

    // device class info is coming from pci manager to save a PCI read
    this->devtype_.reg = devclass;
  }

  uint32_t PCI_Device::read_dword(const uint16_t pci_addr, const uint8_t reg) noexcept {
    PCI::msg req;

    req.data = 0x80000000;
    req.addr = pci_addr;
    req.reg  = reg;

    outpd(PCI::CONFIG_ADDR, 0x80000000 | req.data);
    return inpd(PCI::CONFIG_DATA);
  }
  void PCI_Device::write_dword(const uint8_t reg, const uint32_t value) noexcept {
    PCI::msg req;

    req.data = 0x80000000;
    req.addr = pci_addr_;
    req.reg  = reg;

    outpd(PCI::CONFIG_ADDR, 0x80000000 | req.data);
    outpd(PCI::CONFIG_DATA, value);
  }

  uint32_t PCI_Device::read32(const uint8_t reg) noexcept {
    PCI::msg req;

    req.data = 0x80000000;
    req.addr = pci_addr_;
    req.reg  = reg;

    outpd(PCI::CONFIG_ADDR, 0x80000000 | req.data);
    return inpd(PCI::CONFIG_DATA);
  }

  __attribute__((noinline))
  uint16_t PCI_Device::read16(const uint8_t reg) noexcept {
    PCI::msg req;
    req.data = 0x80000000;
    req.addr = pci_addr_;
    req.reg  = reg;

    outpd(PCI::CONFIG_ADDR, 0x80000000 | req.data);
    uint16_t data = inpw(PCI::CONFIG_DATA + (reg & 2));
    return data;
  }
  void PCI_Device::write16(const uint8_t reg, const uint16_t value) noexcept {
    PCI::msg req;
    req.data = 0x80000000;
    req.addr = pci_addr_;
    req.reg  = reg;

    outpd(PCI::CONFIG_ADDR, 0x80000000 | req.data);
    outpw(PCI::CONFIG_DATA + (reg & 2), value);
  }

  union capability_t
  {
    struct
    {
      uint8_t  id;
      uint8_t  next;
      uint16_t data;
    };
    uint32_t capd;
  };

  void PCI_Device::parse_capabilities()
  {
    caps = {};
    // the capability list is only available if bit 4
    // in the status register is set
    uint16_t status = read16(PCI_STATUS_REG);
    //printf("read16 %#x  status %#x\n", PCI_STATUS_REG, status);
    if ((status & 0x10) == 0) return;
    // this offset works for non-cardbus bridges
    uint32_t offset = PCI_CAPABILITY_REG;
    // read first capability
    offset = read16(offset) & 0xff;
    offset &= ~0x3; // lower 2 bits reserved

    while (offset) {
      capability_t cap;
      cap.capd = read32(offset);
      // remember capability
      this->caps.at(cap.id) = offset;
      // go to next cap
      offset = cap.next;
    }
  }

  void PCI_Device::deactivate()
  {
    // disables device (except for configuration)
    write_dword(PCI_CMD_REG, 0);
  }

  void PCI_Device::intx_enable()
  {
    auto cmd = read16(PCI_CMD_REG);
    write16(PCI_CMD_REG, cmd & ~(1 << 10));
    // delete msi-x
    if (this->msix) delete this->msix;
  }
  bool PCI_Device::intx_status()
  {
    auto stat = read16(PCI_STATUS_REG);
    return stat & (1 << 3);
  }

  std::string PCI_Device::to_string() const
  {
    char buffer[512];
    int len = 0;
    switch (PCI::classcode(devtype_.classcode)) {
    case PCI::classcode::BRIDGE:
      len = snprintf(buffer, sizeof(buffer), "%s, %s, %s (%#06x)",
            PCI::classcode_str(classcode()),
            PCI::vendor_str(vendor_id()),
            (devtype_.subclass < bridge_subclasses.size()
              ? bridge_subclasses[devtype_.subclass] : bridge_subclasses.back()),
            devtype_.subclass);
      break;
    case PCI::classcode::NIC:
      len = snprintf(buffer, sizeof(buffer), "%s, %s, %s (%#x:%#06x)",
            PCI::classcode_str(devtype_.classcode),
            PCI::vendor_str(vendor_id()),
            (devtype_.subclass < nic_subclasses.size()
              ? nic_subclasses[devtype_.subclass] : nic_subclasses.back()),
            this->vendor_id(), this->product_id());
      break;

    case PCI::classcode::STORAGE:
    default:
      len = snprintf(buffer, sizeof(buffer), "%s, %s (%#x:%#06x)",
            PCI::classcode_str(devtype_.classcode),
            PCI::vendor_str(vendor_id()),
            vendor_id(), product_id());
    } //< switch (devtype_.classcode)

    return std::string(buffer, len);
  }

} //< namespace hw
