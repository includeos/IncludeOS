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

#ifndef HW_DEV_HPP
#define HW_DEV_HPP

#include <common>
#include <virtio/virtionet.hpp>
#include <virtio/block.hpp>
#include <virtio/console.hpp>
#include <kernel/pci_manager.hpp>

#include "nic.hpp"
#include "pit.hpp"
#include "disk.hpp"

namespace hw {

  /** @Todo: Implement */
  class Serial;
  class APIC;
  class HPET;

  /**
   *  Access point for registered devices
   */
  class Devices {
  public:

    /**
     * @brief Retreive Nic on position N
     *
     * @note Throws if N is not registered
     *
     * @param N PCI Address
     * @return Reference to the given Nic
     */
    static Nic& nic(const int N)
    { return get<Nic>(N); }

    static Disk<VirtioBlk>& disk(const int N)
    { return get<Disk<VirtioBlk>>(N); }

    /** Get disk N using driver DRIVER */
    template <int N, typename DRIVER, typename... Args>
    static Disk<DRIVER>& disk(Args&&... args) {
      static Disk<DRIVER>
        disk_ {
        PCI_manager::device<PCI::STORAGE>(N),
          std::forward<Args>(args)...
          };
      return disk_;
    }

    /** Get console N using driver DRIVER */
    /*
    template <int N, typename DRIVER>
    static DRIVER& console() {
      static DRIVER con_ {PCI_manager::device<PCI::COMMUNICATION>(N)};
      return con_;
    }
    */

    /**
     *  Get serial port n
     *
     *  @Todo: Make a serial port class, and move rsprint / rswrite etc. from OS out to it.
     *
     *  @Note: The DRIVER parameter is there to support virtio serial ports.
     */
    template <typename DRIVER>
    static PCI_Device& serial(int n);

  private:

    /**
     * @brief Retreive reference to a given
     * @details [long description]
     *
     * @param N [description]
     * @return [description]
     */
    template <typename Device_type>
    inline static Device_type& get(const int N)
    { return *(devices<Device_type>().at(N)); }

    template <typename Device_type>
    using Device_registery = std::vector< std::unique_ptr<Device_type> >;

    template <typename Device_type>
    inline static Device_registery<Device_type>& devices() {
      static Device_registery<Device_type> devices_;
      return devices_;
    }

    /**
     * @brief Register the given device
     * @details
     *
     * @note Private and restriced to only be used by friend classes
     *
     * @param  A unique_ptr to a specific device
     */
    template <typename Device_type>
    static void register_device(std::unique_ptr<Device_type> dev) {
      devices<Device_type>().emplace_back(std::move(dev));
      INFO("Devices", "Registered %s [%u]",
        dev->type_name(), devices<Device_type>().size()-1);
    }

    /** Following classes are allowed to register a device */
    friend class ::PCI_manager;

  }; //< class Devices

  class DeviceNotFound : public std::out_of_range {

  };

} //< namespace hw


#endif //< HW_DEV_HPP
