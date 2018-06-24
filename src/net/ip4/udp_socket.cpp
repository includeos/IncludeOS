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

#include <net/ip4/udp_socket.hpp>
#include <common>
#include <memory>

namespace net
{
  UDPSocket::UDPSocket(UDP& udp_instance, Socket socket)
    : udp_{udp_instance}, socket_{socket}
  {}

  void UDPSocket::packet_init(
      UDP::Packet_ptr p,
      addr_t srcIP,
      addr_t destIP,
      port_t port,
      uint16_t length)
  {
    p->init(this->local_port(), port);
    p->set_ip_src(srcIP.v4());
    p->set_ip_dst(destIP.v4());
    p->set_data_length(length);

    assert(p->data_length() == length);
  }

  void UDPSocket::internal_read(const PacketUDP& udp)
  { on_read_handler(udp.ip_src(), udp.src_port(), (const char*) udp.data(), udp.data_length()); }

  void UDPSocket::sendto(
     addr_t destIP,
     port_t port,
     const void* buffer,
     size_t length,
     sendto_handler cb,
     error_handler ecb)
  {
    if (UNLIKELY(length == 0)) return;
    udp_.sendq.emplace_back(
       (const uint8_t*) buffer, length, cb, ecb, this->udp_,
       local_addr(), this->local_port(), destIP, port);

    // UDP packets are meant to be sent immediately, so try flushing
    udp_.flush();
  }

  void UDPSocket::bcast(
    addr_t srcIP,
    port_t port,
    const void* buffer,
    size_t length,
    sendto_handler cb,
    error_handler ecb)
  {
    if (UNLIKELY(length == 0)) return;
    udp_.sendq.emplace_back(
         (const uint8_t*) buffer, length, cb, ecb, this->udp_,
         srcIP, this->local_port(), IP4::ADDR_BCAST, port);

    // UDP packets are meant to be sent immediately, so try flushing
    udp_.flush();
  }
} // < namespace net
