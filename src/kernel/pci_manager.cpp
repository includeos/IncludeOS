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
#include <vector>

#include <kernel/pci_manager.hpp>
#include <hw/devices.hpp>
#include <hw/pci_device.hpp>

template <typename Driver>
using Driver_entry = std::pair<uint32_t, Driver>;
template <typename Driver>
using fixed_factory_t = std::vector<Driver_entry<Driver>>;

static std::vector<hw::PCI_Device> devices_;
static std::vector<Driver_entry<PCI_manager::NIC_driver>> nic_fact;
static std::vector<Driver_entry<PCI_manager::BLK_driver>> blk_fact;

template <typename Factory, typename Class>
static inline bool register_device(hw::PCI_Device& dev,
                                   fixed_factory_t<Factory>& factory) {
  for (const auto& fact : factory) {
    if (fact.first == dev.vendor_product())
    {
      INFO2("|  +-o Driver found, initializing ");
      INFO2("|");

      auto driver = fact.second(dev);
      hw::Devices::register_device<Class>(std::move(driver));
      return true;
    }
  }
  INFO2("|  +-x Driver not found ");
  return false;
}

/** TEMP FIX until C++17 if constexpr support **/
static inline bool
register_device_nic(hw::PCI_Device& dev, fixed_factory_t<PCI_manager::NIC_driver>& factory)
{
  for (auto& fact : factory) {
    if (fact.first == dev.vendor_product())
    {
      INFO2("|  +-o Driver found, initializing ");
      INFO2("|");

      const int idx = hw::Devices::device_next_index<hw::Nic>();
      auto driver = fact.second(dev, hw::Nic::MTU_detection_override(idx, 1500));
      hw::Devices::register_device<hw::Nic>(std::move(driver));
      return true;
    }
  }
  INFO2("|  +-x Driver not found ");
  return false;
}

PCI_manager::Device_vector PCI_manager::devices () {
  Device_vector device_vec;
  for (const auto& dev : devices_)
    device_vec.push_back(&dev);
  return device_vec;
}

void PCI_manager::scan_bus(const uint8_t classcode, const int bus)
{
  for (uint16_t device = 0; device < 255; ++device)
  {
    const uint16_t pci_addr = bus * 256 + device;
    const uint32_t id = hw::PCI_Device::read_dword(pci_addr, PCI::CONFIG_VENDOR);

    if (id != PCI::WTF)
    {
      hw::PCI_Device::class_revision_t devclass;
      devclass.reg = hw::PCI_Device::read_dword(pci_addr, PCI::CONFIG_CLASS_REV);

      // translate classcode to device and register
      switch (devclass.classcode) {
      case PCI::STORAGE:
        if (classcode == PCI::STORAGE) {
          auto& stored_dev = devices_.emplace_back(pci_addr, id, devclass.reg);
          register_device<BLK_driver, hw::Block_device>(stored_dev, blk_fact);
        }
        break;
      case PCI::NIC:
        if (classcode == PCI::NIC) {
          auto& stored_dev = devices_.emplace_back(pci_addr, id, devclass.reg);
          register_device_nic(stored_dev, nic_fact);
        }
        break;
      case PCI::BRIDGE:
        // scan secondary bus for PCI-to-PCI bridges
        if (devclass.subclass == 0x4) {
          uint16_t buses =
            hw::PCI_Device::read_dword(pci_addr, 0x18);
          scan_bus(classcode, buses >> 8); // secondary is bits 8-15
        }
        break;
      default:
          break;
      }
    }
  }
}

void PCI_manager::init(const uint8_t classcode)
{
  INFO("PCI Manager", "Probing PCI bus");

  /**
   * Probe the PCI buses
   * Starting with the first bus
  **/
  scan_bus(classcode, 0);

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
  nic_fact.emplace_back(driver_id(vendor, prod), factory);
}
void PCI_manager::register_blk(uint16_t vendor, uint16_t prod, BLK_driver factory)
{
  blk_fact.emplace_back(driver_id(vendor, prod), factory);
}
