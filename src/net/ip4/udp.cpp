// Part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015 Oslo and Akershus University College of Applied Sciences
// and  Alfred Bratterud
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

using namespace net;

int UDP::bottom(Packet_ptr pckt)
{
  debug("<UDP handler> Got data");
  std::shared_ptr<PacketUDP> udp = 
      std::static_pointer_cast<PacketUDP> (pckt);
  
  debug("\t Source port: %i, Dest. Port: %i Length: %i\n",
        udp->src_port(), udp->dst_port(), udp->length());
  
  auto it = ports.find(udp->dst_port());
  if (it != ports.end())
  {
    debug("<UDP> Someone's listening to this port. Forwarding...\n");
    return it->second.internal_read(udp);
  }
  
  debug("<UDP> Nobody's listening to this port. Drop!\n");
  return -1;
}

UDP::Socket& UDP::bind(port_t port)
{
  debug("<UDP> Binding to port %i\n", port);
  /// ... !!!
  auto it = ports.find(port);
  if (it == ports.end())
  {
    // create new socket
    auto res = ports.emplace(
			     std::piecewise_construct,
			     std::forward_as_tuple(port),
			     std::forward_as_tuple(stack, port));
    it = res.first;
  }
  return it->second;
}
UDP::Socket& UDP::bind()
{  

  if (ports.size() >= 0xfc00)
    panic("UPD Socket: All ports taken!");  

  debug("UDP finding free ephemeral port\n");  
  while (ports.find(++currentPort) != ports.end())
    if (currentPort  == 0) currentPort = 1025; // prevent automatic ports under 1024
  
  debug("UDP binding to %i port\n", currentPort);
  return bind(currentPort);
}

int UDP::transmit(std::shared_ptr<PacketUDP> udp)
{
  debug2("<UDP> Transmitting %i bytes (seg=%i) from %s to %s:%i\n",
        udp->length(), udp->ip4_segment_size(),
        udp->src().str().c_str(),
        udp->dst().str().c_str(), udp->dst_port());
  
  assert(udp->length() >= sizeof(UDP::udp_header));
  assert(udp->protocol() == IP4::IP4_UDP);
  
  Packet_ptr pckt = Packet::packet(udp);
  return _network_layer_out(pckt);
}

namespace net
{
  int ignore_udp(Packet_ptr)
  {
    debug("<UDP->Network> No handler - DROP!\n");
    return 0;
  }
}
