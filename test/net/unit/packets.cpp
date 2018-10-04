// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015-2017 Oslo and Akershus University College of Applied Sciences
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

#include <common.cxx>
#include <net/buffer_store.hpp>
#include <net/packet.hpp>

using namespace net;
#define BUFFER_CNT 128
#define BUFSIZE 2048

static BufferStore bufstore(BUFFER_CNT, BUFSIZE);

extern "C" __attribute__((weak))
void panic(const char*) { throw std::runtime_error("panic()"); }

CASE("Create empty packet")
{
  auto* ptr = (net::Packet*) bufstore.get_buffer();
  new (ptr) net::Packet(0, 0, 0, &bufstore);
  net::Packet_ptr packet(ptr);

  // everything is zero
  EXPECT(packet->size() == 0);
  EXPECT(packet->capacity() == 0);
  EXPECT(packet->bufsize() == 0);
  // the end of the packet is also the start and end of data
  EXPECT(packet->buf() == packet->buffer_end());
  EXPECT(packet->buf() == packet->data_end());
}

#define DRIVER_OFFSET    12
#define PACKET_CAPA    1012

static Packet_ptr create_packet() noexcept
{
  auto* ptr = (net::Packet*) bufstore.get_buffer();
  new (ptr) net::Packet(DRIVER_OFFSET, 0, DRIVER_OFFSET + PACKET_CAPA, &bufstore);
  return net::Packet_ptr(ptr);
}

CASE("Create packet from buffer store buffer")
{
  auto packet = create_packet();

  EXPECT(packet->size() == 0);
  EXPECT(packet->capacity() == PACKET_CAPA);
  EXPECT(packet->bufsize() == DRIVER_OFFSET + PACKET_CAPA);
  EXPECT(packet->layer_begin() == packet->data_end());
}

CASE("Increment layer in packet")
{
  auto packet = create_packet();
  packet->increment_layer_begin(24);

  EXPECT(packet->size() == 0);
  EXPECT(packet->capacity() == PACKET_CAPA - 24);
  EXPECT(packet->bufsize() == DRIVER_OFFSET + PACKET_CAPA);
  EXPECT(packet->layer_begin() == packet->data_end());
}

CASE("Fill packet with data")
{
  auto packet = create_packet();
  packet->increment_layer_begin(24);

  while (packet->size() < packet->capacity())
      packet->increment_data_end(1);

  EXPECT(packet->size() == packet->capacity());
  EXPECT(packet->capacity() == PACKET_CAPA - 24);
}

CASE("Fill packet with data, increment layer")
{
  auto packet = create_packet();
  packet->increment_layer_begin(24);
  packet->increment_data_end(24);
  packet->increment_layer_begin(24);

  EXPECT(packet->size() == 0);
  EXPECT(packet->capacity() == PACKET_CAPA - 24 * 2);

  packet->set_data_end(24);

  EXPECT(packet->size() == 24);
  EXPECT(packet->capacity() == PACKET_CAPA - 24 * 2);

  // decrement layer, make sure size remains
  packet->increment_layer_begin(-24);
  EXPECT(packet->size() == 24*2);
}

CASE("Moving packet works")
{
  auto packet = create_packet();
  packet->set_data_end(24);

  auto packet2 = std::move(packet);

  EXPECT(packet2->size() == 24);
  EXPECT(packet2->capacity() == PACKET_CAPA);
}

CASE("Verify packet chaining works")
{
  auto packet = create_packet();
  EXPECT(bufstore.available() == BUFFER_CNT - 1);

  // Chain packets
  for (int i = 0; i < BUFFER_CNT - 1; i++) {
    auto chained_packet = create_packet();
    packet->chain(std::move(chained_packet));
    EXPECT(bufstore.available() == BUFFER_CNT - i - 2);
  }

  // Release
  packet = nullptr;
  EXPECT(bufstore.available() == BUFFER_CNT);

  // Reinitialize the first packet
  packet = create_packet();
  EXPECT(bufstore.available() == BUFFER_CNT - 1);

  // Chain
  for (int i = 0; i < BUFFER_CNT - 1; i++) {
    auto chained_packet = create_packet();
    packet->chain(std::move(chained_packet));
    EXPECT(bufstore.available() == BUFFER_CNT - i - 2);
  }

  // Release one-by-one
  auto tail = std::move(packet);
  size_t i = 0;
  while(tail && i < BUFFER_CNT - 1) {
    EXPECT(bufstore.available() == i);
    tail = tail->detach_tail();
    i++;
  }

  tail = nullptr;
  packet = nullptr;
  EXPECT(bufstore.available() == BUFFER_CNT);
}
