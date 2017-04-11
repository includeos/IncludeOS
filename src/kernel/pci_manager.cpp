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

#include <cassert>
#include <common>
#include <delegate>
#include <stdexcept>
#include <kernel/pci_manager.hpp>
#include <hw/devices.hpp>
#include <hw/pci_device.hpp>
#include <util/fixedvec.hpp>

static const int ELEMENTS = 16;

template <typename Driver>
using Driver_entry = std::pair<uint32_t, Driver>;
template <typename Driver>
using fixed_factory_t = fixedvector<Driver_entry<Driver>, ELEMENTS>;

// PCI devices
fixedvector<hw::PCI_Device, ELEMENTS> devices(Fixedvector_Init::UNINIT);

// driver factories
fixed_factory_t<PCI_manager::NIC_driver> nic_fact(Fixedvector_Init::UNINIT);
fixed_factory_t<PCI_manager::BLK_driver> blk_fact(Fixedvector_Init::UNINIT);

template <typename Factory, typename Class>
static inline bool register_device(hw::PCI_Device& dev, fixed_factory_t<Factory>& fact) {
  debug("vendor: 0x%x prod: 0x%x, id: 0x%x\n",
    dev.vendor_id(), dev.product_id(), driver_id(dev));

  for (auto& driver : fact) {
    if (driver.first == dev.vendor_product())
    {
      INFO2("|  +--+ Driver: Found");

      hw::Devices::register_device<Class>(driver.second(dev));
      return true;
    }
  }
  INFO2("|  +--+ Driver: Not found");
  return false;
}

void PCI_manager::pre_init()
{
  devices.clear();
  nic_fact.clear();
  blk_fact.clear();
}

void PCI_manager::scan_bus(int bus)
{
  for (uint16_t device = 0; device < 255; ++device)
  {
    uint16_t pci_addr = bus * 256 + device;
    uint32_t id =
        hw::PCI_Device::read_dword(pci_addr, PCI::CONFIG_VENDOR);

    if (id != PCI::WTF)
    {
      hw::PCI_Device::class_revision_t devclass;
      devclass.reg =
              hw::PCI_Device::read_dword(pci_addr, PCI::CONFIG_CLASS_REV);

      auto& dev = devices.emplace(pci_addr, id, devclass.reg);

      bool registered = false;
      // translate classcode to device and register
      switch (dev.classcode()) {
      case PCI::STORAGE:
          registered = register_device<BLK_driver, hw::Block_device>(dev, blk_fact);
          break;
      case PCI::NIC:
          registered = register_device<NIC_driver, hw::Nic>(dev, nic_fact);
          break;
      case PCI::BRIDGE:
          // scan secondary bus for PCI-to-PCI bridges
          if (dev.subclass() == 0x4) {
            uint16_t buses = dev.read16(0x18);
            scan_bus(buses >> 8); // secondary is bits 8-15
          }
          break;
      default:
          break;
      }
      debug("Device %s", registered ? "registed" : "not registered");
    }
  }
}

void PCI_manager::init()
{
  INFO("PCI Manager", "Probing PCI bus");

  /**
   * Probe the PCI buses
   * Starting with the first bus
  **/
  scan_bus(0);

  // Pretty printing, end of device tree
  // @todo should probably be moved, or optionally non-printed
  INFO2("|");
  INFO2("o");
}

inline uint32_t driver_id(uint16_t vendor, uint16_t prod) {
  return (uint32_t) prod << 16 | vendor;
}

void PCI_manager::register_nic(uint16_t vendor, uint16_t prod, NIC_driver factory)
{
  nic_fact.emplace(driver_id(vendor, prod), factory);
}
void PCI_manager::register_blk(uint16_t vendor, uint16_t prod, BLK_driver factory)
{
  blk_fact.emplace(driver_id(vendor, prod), factory);
}
