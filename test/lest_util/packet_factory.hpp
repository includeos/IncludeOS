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
#ifndef TEST_PACKET_FACTORY_HPP
#define TEST_PACKET_FACTORY_HPP

#include <net/ethernet/header.hpp>
#include <net/buffer_store.hpp>
#include <net/packet.hpp>

#define BUFFER_CNT   128
#define BUFFER_SIZE 2048

#define PHYS_OFFSET     0
#define PACKET_CAPA  1514

static net::Packet_ptr create_packet() noexcept
{
  static net::BufferStore bufstore(BUFFER_CNT, BUFFER_SIZE);
  auto buffer = bufstore.get_buffer();
  auto* ptr = (net::Packet*) buffer.addr;
  new (ptr) net::Packet(PHYS_OFFSET, 0, PHYS_OFFSET + PACKET_CAPA, buffer.bufstore);
  return net::Packet_ptr(ptr);
}

#endif
