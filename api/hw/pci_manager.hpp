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

#ifndef HW_PCI_MANAGER_HPP
#define HW_PCI_MANAGER_HPP

#include <vector>
#include <cstdio>
#include <unordered_map>

#include "pci_device.hpp"

class PCI_manager {
private:
  using Device_Registry = std::unordered_map<PCI::classcode_t, std::vector<hw::PCI_Device>>;

public:
  template <PCI::classcode_t CLASS>
  static hw::PCI_Device& device(const int n) noexcept {
    return devices_[CLASS][n];
  };

  template <PCI::classcode_t CLASS>
  static size_t num_of_devices() noexcept {
    return devices_[CLASS].size();
  }

private:
  static Device_Registry devices_;

  /**
   *  Keep track of certain devices
   *  
   *  The PCI manager can probe and keep track of devices which can (possibly)
   *  be specialized by the Dev-class later.
   */
  static void init();

  friend class OS;
}; //< class PCI_manager

#endif //< HW_PCI_MANAGER_HPP
