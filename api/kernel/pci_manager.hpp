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

#ifndef KERNEL_PCI_MANAGER_HPP
#define KERNEL_PCI_MANAGER_HPP

#include <vector>
#include <cstdio>
#include <unordered_map>
#include <delegate>

#include <hw/pci_device.hpp>
#include <hw/devices.hpp>

class PCI_manager {
private:
  using Device_registry = std::unordered_map<PCI::classcode_t, std::vector<hw::PCI_Device>>;

public:
  template <PCI::classcode_t CLASS>
  static hw::PCI_Device& device(const int n) noexcept {
    return devices_[CLASS][n];
  };

  template <PCI::classcode_t CLASS>
  static size_t num_of_devices() noexcept {
    return devices_[CLASS].size();
  }

  /** Whats being stored and returned is a unique_ptr of the given device */
  template <typename Device_type>
  using Dev_ptr = std::unique_ptr<Device_type>;

  /** The function (factory) thats create the Dev_ptr from PCI_Device, supplied by the driver */
  template <typename Device_type>
  using Driver_factory = delegate< Dev_ptr<Device_type>(hw::PCI_Device&) >;

  /**
   * @brief Register a specific type of driver factory
   * @details Register a specific type of driver factory
   * (function that creates a pointer to the given Device, with its full driver implementation)
   * Indexed as a combination of vendor + product
   *
   * @param vendor Driver vendor id
   * @param product Driver product id
   * @param driver_factory Function for creating a driver
   * @tparam Device_type The specific type of Device the driver is for
   */
  template <typename Device_type>
  static void register_driver(uint16_t vendor, uint16_t product,
    Driver_factory<Device_type> driver_factory)
  {
    drivers<Device_type>().emplace(get_driver_id(vendor, product), driver_factory);
  }

  /** Currently a combination of Model + Product (we don't care about the revision etc. atm.)*/
  using driver_id_t = uint32_t;

  /** Combine vendor and product id to represent a driver id */
  static driver_id_t get_driver_id(uint16_t vendor, uint16_t product)
  { return (uint32_t)(vendor) << 16 | product; }

  static driver_id_t get_driver_id(hw::PCI_Device& dev)
  { return get_driver_id(dev.vendor_id(), dev.product_id()); }

private:
  static Device_registry devices_;

  /** A register for a specific type of drivers: map[driver_id, driver_factory]*/
  template <typename Device_type>
  using Driver_registry = std::unordered_map<driver_id_t, Driver_factory<Device_type> >;

  /**
   * @brief Retrieve drivers (factories) of a given type of device
   *
   * @return A collection of driver factories indexed by driver_id
   */
  template <typename Device_type>
  static Driver_registry<Device_type>& drivers() {
    static Driver_registry<Device_type> drivers_;
    return drivers_;
  }

  template <typename Device_type>
  inline static bool register_device(hw::PCI_Device& dev);

  /**
   *  Keep track of certain devices
   *
   *  The PCI manager can probe and keep track of devices which can (possibly)
   *  be specialized by the Dev-class later.
   */
  static void init();

  friend class OS;
}; //< class PCI_manager

template <typename Device_type>
inline bool PCI_manager::register_device(hw::PCI_Device& dev) {
  try
  {
    debug("sz=%u, mod: 0x%x prod: 0x%x, id: 0x%x\n",
      drivers<Device_type>().size(), dev.vendor_id(), dev.product_id(),
      get_driver_id(dev));

    auto driver_factory = drivers<Device_type>().at(get_driver_id(dev));
    INFO2("|  +--+ Driver: Found");

    hw::Devices::register_device(driver_factory(dev));
    return true;
  }
  catch(std::out_of_range)
  {
    INFO2("|  +--+ Driver: Not found");
  }
  return false;
}

#endif //< KERNEL_PCI_MANAGER_HPP
