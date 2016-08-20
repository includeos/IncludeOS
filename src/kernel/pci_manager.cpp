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

#define NUM_BUSES 2

PCI_manager::Device_registry PCI_manager::devices_;

void PCI_manager::init() {
  INFO("PCI Manager", "Probing PCI bus");

  /*
   * Probe the PCI bus
   * - Assuming bus number is 0, there are 255 possible addresses
   */
  uint32_t id {PCI::WTF};

  for (uint16_t pci_addr {0}; pci_addr < 255; ++pci_addr) {
    id = hw::PCI_Device::read_dword(pci_addr, PCI::CONFIG_VENDOR);

    if (id != PCI::WTF) {
      hw::PCI_Device dev {pci_addr, id};

      // store device
      devices_[dev.classcode()].emplace_back(dev);

      bool registered = false;
      // translate classcode to device and register
      switch(dev.classcode())
      {
        case PCI::STORAGE:
          registered = register_device<hw::Drive>(dev);
          break;

        case PCI::NIC:
          registered = register_device<hw::Nic>(dev);
          break;

        default:
        {

        }

      }
      debug("Device %s", registered ? "registed":"not registered");
    }
  }

  // Pretty printing, end of device tree
  // @todo should probably be moved, or optionally non-printed
  INFO2("|");
  INFO2("o");
}
