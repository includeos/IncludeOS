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

#ifndef HW_DEVICES_HPP
#define HW_DEVICES_HPP

#include <common>
#include <virtio/console.hpp>

#include "nic.hpp"
#include "pit.hpp"
#include "drive.hpp"

class PCI_manager; // for friending

namespace hw {

  /** @Todo: Implement */
  class Serial;
  class APIC;
  class HPET;

  class Device_not_found;

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

    static Drive& drive(const int N)
    { return get<Drive>(N); }

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

    /** List all devices (decorated, as seen in boot output) */
    inline static void print_devices();


    inline static void deactivate_all();

    /**
     * @brief Retreive reference to the given Device on pos N
     * @details Helper to retreive a Device of a given type
     * on position N.
     *
     * Throws NotFoundException of the device isnt there.
     *
     * @param N position
     * @tparam Device_type type of Device
     * @return Reference to the Device on pos N
     */
    template <typename Device_type>
    inline static Device_type& get(const int N);

    /** Registry of devices of a given type */
    template <typename Device_type>
    using Device_registry = std::vector< std::unique_ptr<Device_type> >;

    /** Returns the device registry of a given type */
    template <typename Device_type>
    inline static Device_registry<Device_type>& devices() {
      static Device_registry<Device_type> devices_;
      return devices_;
    }


  private:


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
      debug("<Devices> Registered %s [%u]",
        dev->device_type(), devices<Device_type>().size()-1);
    }

    /** Print a decorated indexed list with the devices of the given type. No output if empty */
    template <typename Device_type>
    inline static void print_devices(const Device_registry<Device_type>& devices);

    template <typename Device_type>
    inline static void deactivate_type(Device_registry<Device_type>& devices);

    /** Following classes are allowed to register a device */
    friend class ::PCI_manager;

  }; //< class Devices

  /** Exception thrown when a device is not found (registered) */
  class Device_not_found : public std::out_of_range {
  public:
    explicit Device_not_found(const std::string& type, const int n)
      : std::out_of_range(
          std::string{"Device of type "} + type +
          std::string{" not found at position #"}
          + std::to_string(n))
      {}
  }; //< class Device_not_found

  template <typename Device_type>
  inline Device_type& Devices::get(const int N) {
    try {
      return *(devices<Device_type>().at(N));
    }
    catch(std::out_of_range)
    {
      throw Device_not_found{Device_type::device_type(), N};
    }
  }

  template <typename Device_type>
  inline void Devices::print_devices(const Device_registry<Device_type>& devices)
  {
    if(not devices.empty())
    {
      INFO2("|");
      INFO2("+--+ %s", Device_type::device_type());

      for(size_t i = 0; i < devices.size(); i++)
        INFO2("|  + #%u: %s", i, devices[i]->name());
    }
  }

  inline void Devices::print_devices()
  {
    INFO("Devices", "Listing registered devices");

    print_devices(devices<hw::Drive>());
    print_devices(devices<hw::Nic>());

    INFO2("|");
    INFO2("o");
  }

  // helpers to shutdown PCI devices
  template <typename Device_type>
  inline void Devices::deactivate_type(Device_registry<Device_type>& devices)
  {
    for (auto& dev : devices)
        dev->deactivate();
  }
  inline void Devices::deactivate_all()
  {
    deactivate_type(devices<hw::Drive>());
    deactivate_type(devices<hw::Nic>());
  }

} //< namespace hw


#endif //< HW_DEVICES_HPP
