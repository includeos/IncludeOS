// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015-2016 Oslo and Akershus University College of Applied Sciences
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

#include <net/tcp/packet.hpp>
#include <net/ethernet/header.hpp>
#include <net/buffer_store.hpp>
#include <net/packet.hpp>
#include <common.cxx>

using namespace std::string_literals;

extern lest::tests & specification();

CASE("round_up returns number of d-sized chunks required for n")
{
  unsigned res;
  unsigned chunk_size {128};
  res = round_up(127, chunk_size);
  EXPECT(res == 1u);
  res = round_up(128, chunk_size);
  EXPECT(res == 1u);
  res = round_up(129, chunk_size);
  EXPECT(res == 2u);
}

CASE("round_up expects div to be greater than 0")
{
  unsigned chunk_size {0};
  EXPECT_THROWS(round_up(128, chunk_size));
}

using namespace net;
#define BUFFER_CNT   128
#define BUFFER_SIZE 2048
static BufferStore bufstore(BUFFER_CNT, BUFFER_SIZE);

#define DRIVER_OFFSET    12
#define PACKET_CAPA    1012

static Packet_ptr create_packet() noexcept
{
  auto buffer = bufstore.get_buffer();
  auto* ptr = (net::Packet*) buffer.addr;
  new (ptr) net::Packet(DRIVER_OFFSET, 0, DRIVER_OFFSET + PACKET_CAPA, buffer.bufstore);
  return net::Packet_ptr(ptr);
}
static tcp::Packet_ptr create_tcp_packet() noexcept
{
  auto pkt = create_packet();
  pkt->increment_layer_begin(sizeof(net::ethernet::Header));
  /// TCP stuff
  auto tcp = static_unique_ptr_cast<tcp::Packet> (std::move(pkt));
  tcp->init();
  return tcp;
}

CASE("Empty TCP packet")
{
  auto tcp = create_tcp_packet();
  EXPECT(tcp->size() == 40);
  EXPECT(tcp->ip_header_length() == 20);
  EXPECT(tcp->ip_data_length() == 20);

  EXPECT(tcp->tcp_header_length() == 20);
  EXPECT(tcp->tcp_data_length() == 0);
  EXPECT(tcp->has_tcp_data() == false);

  EXPECT(tcp->has_tcp_options() == false);
}

CASE("Fill 1 byte")
{
  auto tcp = create_tcp_packet();
  tcp->fill((const uint8_t*) "data", 1);

  EXPECT(tcp->size() == 41);
  EXPECT(tcp->ip_header_length() == 20);
  EXPECT(tcp->ip_data_length() == 21);

  EXPECT(tcp->tcp_header_length() == 20);
  EXPECT(tcp->tcp_data_length() == 1);
  EXPECT(tcp->has_tcp_data() == true);

  EXPECT(tcp->has_tcp_options() == false);
  EXPECT(tcp->tcp_options_length() == 0);
}

struct Optijohn {
  Optijohn(uint8_t v) : value(v) {}
  const uint8_t   kind    {0xFF};
  const uint8_t   length  {1};
  uint8_t value;
};

CASE("Add 1 option")
{
  auto tcp = create_tcp_packet();
  tcp->add_tcp_option<Optijohn>(1);

  EXPECT(tcp->size() == 44);
  EXPECT(tcp->ip_header_length() == 20);
  EXPECT(tcp->ip_data_length() == 24);

  EXPECT(tcp->tcp_header_length() == 24);
  EXPECT(tcp->tcp_data_length() == 0);
  EXPECT(tcp->has_tcp_data() == false);

  EXPECT(tcp->has_tcp_options() == true);
  EXPECT(tcp->tcp_options_length() == 4);
}

CASE("Add too many options")
{
  auto tcp = create_tcp_packet();
  for (int i = 0; i < 10; i++)
      tcp->add_tcp_option<Optijohn>(i);

  EXPECT_THROWS(tcp->add_tcp_option<Optijohn>(11));

  EXPECT(tcp->size() == 80);
  EXPECT(tcp->ip_header_length() == 20);
  EXPECT(tcp->ip_data_length() == 60);

  EXPECT(tcp->tcp_header_length() == 60);
  EXPECT(tcp->tcp_data_length() == 0);
  EXPECT(tcp->has_tcp_data() == false);

  EXPECT(tcp->has_tcp_options() == true);
  EXPECT(tcp->tcp_options_length() == 40);
}
