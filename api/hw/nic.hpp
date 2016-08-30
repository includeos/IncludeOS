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

#include "../net/inet_common.hpp"
#include "../net/buffer_store.hpp"
#include "../net/ethernet.hpp"

namespace hw {

  /**
   *  A public interface for Ethernet Network cards
   *
   *  This interface assumes network card is of type Ethernet
   */
  class Nic {
  public:

    /** Get a readable name. */
    virtual const char* name() const = 0;

    /** A readable name of the type of device @todo: move to a abstract Device? */
    static const char* device_type()
    { return "NIC"; }

    /**
      The mac address.
      @todo remove depedency for Ethernet (somewhere in the future)
    */
    virtual const net::Ethernet::addr& mac() = 0;

    virtual uint16_t MTU() const noexcept = 0;

    net::BufferStore& bufstore() noexcept
    { return bufstore_; }

    size_t buffers_available()
    { return bufstore_.available(); }

    uint16_t bufsize() const
    { return bufstore_.bufsize(); }

    uint16_t eth_size() const
    { return sizeof(net::Ethernet::header) + sizeof(net::Ethernet::trailer); }


    /** Delegate linklayer output. Hooks into IP-stack bottom, w.UPSTREAM data. */
    void set_linklayer_out(net::upstream link_out)
    { _link_out = link_out; };

    net::upstream get_linklayer_out()
    { return _link_out; }

    virtual void transmit(net::Packet_ptr pckt) = 0;

    /** Let the driver return a delegate to receive outgoing packets from layer above */
    virtual net::downstream get_physical_out() = 0;

    /** Subscribe to event for when there is more room in the tx queue */
    void on_transmit_queue_available(net::transmit_avail_delg del)
    { transmit_queue_available_event_ = del; }

    virtual size_t transmit_queue_available() = 0;

    virtual size_t receive_queue_waiting() = 0;

    std::string ifname() const {
      return "eth" + std::to_string(N);
    }

  protected:
    /**
     *  Constructor
     *
     *  Constructed by the actual Nic Driver
     */
    Nic(uint32_t bufstore_sz, uint16_t bufsz)
      : bufstore_{ bufstore_sz, bufsz }
    {
      static int id_counter = 0;
      N = id_counter++;
    }

    friend class Devices;

    /** Upstream delegate for linklayer output */
    net::upstream _link_out;
    net::transmit_avail_delg transmit_queue_available_event_ =
      [](auto) { assert(0 && "<NIC> Transmit queue available delegate is not set!"); };

  private:
    net::BufferStore bufstore_;
    int N;

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
