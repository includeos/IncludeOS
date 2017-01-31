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

#include <common>

#include <kernel/pci_manager.hpp>
#include <hw/devices.hpp>
#include <stdexcept>

PCI_manager::Device_registry PCI_manager::devices_;

void PCI_manager::scan_bus(int bus)
{
  for (uint16_t device = 0; device < 255; ++device)
  {
    uint16_t pci_addr = bus * 256 + device;
    uint32_t id = 
        hw::PCI_Device::read_dword(pci_addr, PCI::CONFIG_VENDOR);

    if (id != PCI::WTF)
    {
      // needed for classcode
      hw::PCI_Device::class_revision devclass;
      devclass.reg = 
          hw::PCI_Device::read_dword(pci_addr, PCI::CONFIG_CLASS_REV);
      // convert to annoying enum :-)
      auto classcode = (PCI::classcode_t) devclass.classcode;

      // store device directly into map
      devices_[classcode].emplace_back(pci_addr, id, devclass.reg);
      auto& dev = devices_[classcode].back();

      bool registered = false;
      // translate classcode to device and register
      switch (dev.classcode()) {
      case PCI::STORAGE:
          registered = register_device<hw::Block_device>(dev);
          break;
      case PCI::NIC:
          registered = register_device<hw::Nic>(dev);
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
