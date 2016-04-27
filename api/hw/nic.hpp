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

#ifndef HW_NIC_HPP
#define HW_NIC_HPP

#include "pci_device.hpp"

#include "../net/ethernet.hpp"
#include "../net/inet_common.hpp"
#include "../net/buffer_store.hpp"

namespace hw {

  /**
   *  A public interface for Network cards
   *
   *  @note: The requirements for a driver is implicitly given by how it's used below,
   *         rather than explicitly by inheritance. This avoids vtables.
   *
   *  @note: Drivers are passed in as template paramter so that only the drivers
   *         we actually need will be added to our project.
   */
  template <typename DRIVER>
  class Nic {
  public:
    using driver_t = DRIVER;

    /** Get a readable name. */
    inline const char* name() const noexcept
    { return driver_.name(); }


    /** The mac address. */
    inline const net::Ethernet::addr& mac()
    { return driver_.mac(); }

    inline void set_linklayer_out(net::upstream del)
    { driver_.set_linklayer_out(del); }

    inline net::upstream get_linklayer_out()
    { return driver_.get_linklayer_out(); }

    inline void transmit(net::Packet_ptr pckt)
    { driver_.transmit(pckt); }

    inline uint16_t MTU() const noexcept
    { return driver_.MTU(); }

    inline uint16_t bufsize() const noexcept
    { return driver_.bufsize(); }

    inline net::BufferStore& bufstore() noexcept
    { return driver_.bufstore(); }

    inline void on_transmit_queue_available(net::transmit_avail_delg del)
    { driver_.on_transmit_queue_available(del); }

    inline size_t transmit_queue_available()
    { return driver_.transmit_queue_available(); }

    inline size_t receive_queue_waiting(){
      return driver_.receive_queue_waiting();
    };

    inline size_t buffers_available()
    { return bufstore().buffers_available(); }

    inline void on_exit_to_physical(delegate<void(net::Packet_ptr)> dlg)
    { driver_.on_exit_to_physical(dlg); }

  private:
    driver_t driver_;

    /**
     *  Constructor
     *
     *  Just a wrapper around the driver constructor.
     *
     *  @note: The Dev-class is a friend and will call this
     */
    Nic(PCI_Device& d) : driver_{d} {}

    friend class Dev;
  };

  /** Future drivers may start out like so, */
  class E1000 {
  public:
    inline const char* name() const noexcept
    { return "E1000 Driver"; }
    //...whatever the Nic class implicitly needs
  };

  /** Hopefully somebody will port a driver for this one */
  class RTL8139;

} //< namespace hw

#endif // NIC_HPP
