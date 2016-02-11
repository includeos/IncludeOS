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

#define DEBUG
#include <os>
#include <net/ip4/udp.hpp>
#include <net/util.hpp>
#include <memory>

namespace net {

void UDP::bottom(Packet_ptr pckt)
{
  debug("<UDP handler> Got data");
  std::shared_ptr<PacketUDP> udp = 
      std::static_pointer_cast<PacketUDP> (pckt);
  
  debug("\t Source port: %i, Dest. Port: %i Length: %i\n",
        udp->src_port(), udp->dst_port(), udp->length());
  
  auto it = ports_.find(udp->dst_port());
  if (it != ports_.end())
  {
    debug("<UDP> Someone's listening to this port. Forwarding...\n");
    it->second.internal_read(udp);
  }
  
  debug("<UDP> Nobody's listening to this port. Drop!\n");
}

UDP::Socket& UDP::bind(port_t port)
{
  debug("<UDP> Binding to port %i\n", port);
  /// ... !!!
  auto it = ports_.find(port);
  if (it == ports_.end()) {
    // create new socket
    auto res = ports_.emplace(
			     std::piecewise_construct,
			     std::forward_as_tuple(port),
			     std::forward_as_tuple(stack_, port));
    it = res.first;
  }
  return it->second;
}

UDP::Socket& UDP::bind() {  

  if (ports_.size() >= 0xfc00)
    panic("UPD Socket: All ports taken!");  

  debug("UDP finding free ephemeral port\n");  
  while (ports_.find(++current_port_) != ports_.end())
    if (current_port_  == 0) current_port_ = 1025; // prevent automatic ports under 1024
  
  debug("UDP binding to %i port\n", current_port_);
  return bind(current_port_);
}

void UDP::transmit(std::shared_ptr<PacketUDP> udp) {
  debug2("<UDP> Transmitting %i bytes (seg=%i) from %s to %s:%i\n",
        udp->length(), udp->ip4_segment_size(),
        udp->src().str().c_str(),
        udp->dst().str().c_str(), udp->dst_port());
  
  assert(udp->length() >= sizeof(UDP::udp_header));
  assert(udp->protocol() == IP4::IP4_UDP);
  
  Packet_ptr pckt = Packet::packet(udp);
  network_layer_out_(pckt);
}

void ignore_udp(Packet_ptr)
{
  debug("<UDP->Network> No handler - DROP!\n");
}

} //< namespace net
