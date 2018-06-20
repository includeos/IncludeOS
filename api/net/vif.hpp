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
#ifndef NET_VIF_HPP
#define NET_VIF_HPP

#include "link_layer.hpp"
#include <map>

namespace net {
/**
 * @brief      Virtual Network Interface (kinda)
 */
template <typename Protocol>
class Vif : public Link_layer<Protocol> {
public:
  using Linklayer  = Link_layer<Protocol>;

  Vif(hw::Nic& link, const int id)
    : Linklayer(Protocol{{this, &Vif<Protocol>::transmit}, link.mac(), id}),
      link_{link}, id_{id}, phys_down_{link_.create_physical_downstream()}
  {}

  int id() const noexcept
  { return id_; }

  void set_physical_downstream(net::downstream phys_down)
  { phys_down_ = phys_down; }

  void transmit(Packet_ptr pkt)
  {
    //printf("<Vif> Transmitting to physical\n");
    Expects(phys_down_);
    phys_down_(std::move(pkt));
  }

  void receive(Packet_ptr pkt)
  { Linklayer::receive(std::move(pkt)); }

  net::downstream create_physical_downstream() override
  { return phys_down_; }

  const char* driver_name() const override
  { return "Virtual Interface"; }

  std::string device_name() const override
  { return link_.device_name() + "." + std::to_string(id_); }

  const MAC::Addr& mac() const noexcept override
  { return link_.mac(); }

  uint16_t MTU() const noexcept override
  { return link_.MTU(); }

  net::Packet_ptr create_packet(int layer_begin) override
  { return link_.create_packet(layer_begin); }

  void on_transmit_queue_available(net::transmit_avail_delg del) override
  { link_.on_transmit_queue_available(del); }

  size_t transmit_queue_available() override
  { return link_.transmit_queue_available(); }

  void deactivate() override
  { }

  void move_to_this_cpu() override
  { link_.move_to_this_cpu(); }

  void flush() override
  { link_.flush(); }

  void poll() override
  { link_.poll(); }

private:
  hw::Nic& link_;
  const int id_;
  net::downstream phys_down_ = nullptr;

};

} // < namespace net

#endif
