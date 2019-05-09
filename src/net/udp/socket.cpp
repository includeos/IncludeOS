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

#include <net/udp/socket.hpp>
#include <net/udp/udp.hpp>
#include <common>
#include <memory>

namespace net::udp
{
  Socket::Socket(UDP& udp_instance, net::Socket socket)
    : udp_{udp_instance}, socket_{std::move(socket)},
      is_ipv6_{socket.address().is_v6()}
  {}

  void Socket::internal_read(const Packet_view& udp)
  {
    on_read_handler(udp.ip_src(), udp.src_port(),
                   (const char*) udp.udp_data(), udp.udp_data_length());
  }

  void Socket::sendto(
     addr_t destIP,
     port_t port,
     const void* buffer,
     size_t length,
     sendto_handler cb,
     error_handler ecb)
  {
    if (UNLIKELY(length == 0)) return;
    udp_.sendq.emplace_back(this->udp_,
      socket_, net::Socket{destIP, port},
      (const uint8_t*) buffer, length,
      cb, ecb);

    // UDP packets are meant to be sent immediately, so try flushing
    udp_.flush();
  }

  void Socket::bcast(
    addr_t srcIP,
    port_t port,
    const void* buffer,
    size_t length,
    sendto_handler cb,
    error_handler ecb)
  {
    // TODO: IPv6 support?
    Expects(not is_ipv6_ && "Don't know what broadcast with IPv6 is yet");
    if (UNLIKELY(length == 0)) return;
    udp_.sendq.emplace_back(this->udp_,
      net::Socket{srcIP, socket_.port()}, net::Socket{ip4::Addr::addr_bcast, port},
      (const uint8_t*) buffer, length,
      cb, ecb);

    // UDP packets are meant to be sent immediately, so try flushing
    udp_.flush();
  }

  void Socket::close()
  {
    udp_.close(socket_);
  }
} // < namespace net
