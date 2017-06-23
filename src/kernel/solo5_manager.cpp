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
#include <delegate>

#include <kernel/solo5_manager.hpp>
#include <hw/devices.hpp>
#include <hw/pci_device.hpp>
#include <stdexcept>
#include <util/fixedvec.hpp>

static const int ELEMENTS = 16;

// PCI devices
fixedvector<hw::PCI_Device, ELEMENTS> devices(Fixedvector_Init::UNINIT);

void Solo5_manager::init() {
  INFO("Solo5", "Looking for solo5 devices");

  uint32_t id_net = 0x1000 << 16 | PCI::VENDOR_SOLO5;
  uint32_t id_blk = 0x1001 << 16 | PCI::VENDOR_SOLO5;

  auto& stored_blk = devices.emplace(PCI::SOLO5_BLK_DUMMY_ADDR, id_blk, 0);
  auto& stored_net = devices.emplace(PCI::SOLO5_NET_DUMMY_ADDR, id_net, 0);

  register_device<hw::Block_device>(stored_blk);
  register_device<hw::Nic>(stored_net);
}
