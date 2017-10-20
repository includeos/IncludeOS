// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2017 Oslo and Akershus University College of Applied Sciences
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

#pragma once
#ifndef NET_LINK_LAYER_HPP
#define NET_LINK_LAYER_HPP

#include <hw/nic.hpp>

namespace net {

template <class T>
class Link_layer : public hw::Nic {
public:
  using Protocol    = T;
  using upstream    = hw::Nic::upstream;
  using downstream_link  = hw::Nic::downstream;
public:
  explicit Link_layer(Protocol&& protocol, BufferStore& bufstore);

  std::string device_name() const override {
    return link_.link_name();
  }

  downstream_link create_link_downstream() override
  { return {link_, &Protocol::transmit}; }

  net::upstream_ip& ip4_upstream() override
  { return link_.ip4_upstream(); }

  net::upstream_ip& ip6_upstream() override
  { return link_.ip6_upstream(); }

  upstream& arp_upstream() override
  { return link_.arp_upstream(); }

  void set_ip4_upstream(upstream_ip handler) override
  { link_.set_ip4_upstream(handler); }

  void set_ip6_upstream(upstream_ip handler) override
  { link_.set_ip6_upstream(handler); }

  void set_arp_upstream(upstream handler) override
  { link_.set_arp_upstream(handler); }

  void set_vlan_upstream(upstream handler) override
  { link_.set_vlan_upstream(handler); }

  /** Number of bytes in a frame needed by the device itself **/
  virtual size_t frame_offset_device() override
  { return 0; }

  /** Number of bytes in a frame needed by the linklayer **/
  virtual size_t frame_offset_link() override
  { return Protocol::header_size(); }

  hw::Nic::Proto proto() const override
  { return Protocol::proto(); }

  /** Stats getters **/
  uint64_t get_packets_rx() override
  { return link_.get_packets_rx(); }

  uint64_t get_packets_tx() override
  { return link_.get_packets_tx(); }

  uint64_t get_packets_dropped() override
  { return link_.get_packets_dropped(); }


protected:
  /** Called by the underlying physical driver inheriting the Link_layer */
  void receive(net::Packet_ptr pkt)
  { link_.receive(std::move(pkt)); }

private:
  Protocol link_;
};

template <class Protocol>
Link_layer<Protocol>::Link_layer(Protocol&& protocol, BufferStore& bufstore)
  : hw::Nic(bufstore),
    link_{std::forward<Protocol>(protocol)}
{
}

} // < namespace net

#endif
