// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2018 IncludeOS AS, Oslo, Norway
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

#include <delegate>
#include <net/addr.hpp>
#include <net/packet.hpp>
#include <net/error.hpp>

namespace net {
  // temp
  class PacketUDP;
}
namespace net::udp
{
  using addr_t = net::Addr;
  using port_t = uint16_t;

  using sendto_handler    = delegate<void()>;
  using error_handler     = delegate<void(const Error&)>;

  // temp
  using Packet_ptr = std::unique_ptr<PacketUDP, std::default_delete<net::Packet>>;

}
