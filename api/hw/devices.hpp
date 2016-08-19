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
    static Nic& nic(const int N);

    static Disk<VirtioBlk>& disk(const int N);

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
     * @brief Register the given device
     * @details Checks the vendor and type
     * and registers the device as the correct type with the correct driver.
     *
     * @note Currently restricted to only be used by PCI_manager
     *
     * @param  A PCI Device
     */
    static void register_device(PCI_Device&);

    friend class ::PCI_manager;

  }; //< class Devices

} //< namespace hw


#endif //< HW_DEV_HPP
