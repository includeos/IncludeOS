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

#include <packet_factory.hpp>
#include <net/tcp/packet.hpp>
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

CASE("Empty TCP packet")
{
  auto tcp = create_tcp_packet();
  tcp->init();
  EXPECT(tcp->size() == 40);
  EXPECT(tcp->ip_capacity() == 1480);
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
  tcp->init();
  EXPECT(tcp->fill((const uint8_t*) "data", 1) == 1);

  EXPECT(tcp->size() == 41);
  EXPECT(tcp->ip_header_length() == 20);
  EXPECT(tcp->ip_data_length() == 21);

  EXPECT(tcp->tcp_header_length() == 20);
  EXPECT(tcp->tcp_data_length() == 1);
  EXPECT(tcp->has_tcp_data() == true);

  EXPECT(tcp->has_tcp_options() == false);
  EXPECT(tcp->tcp_options_length() == 0);
}

CASE("Fill too many bytes")
{
  uint8_t buffer[9000];
  strcpy((char*) buffer, "data here!");

  auto tcp = create_tcp_packet();
  tcp->init();
  EXPECT(tcp->fill(buffer, sizeof(buffer)) == 1460);
  // packet should be full now
  EXPECT(tcp->fill(buffer, sizeof(buffer)) == 0);
  EXPECT(tcp->ip_capacity() == 1480);

  EXPECT(tcp->ip_data_length() == 1480);
  EXPECT(tcp->tcp_data_length() == 1460);

  EXPECT(strcmp((const char*) tcp->tcp_data(), "data here!") == 0);
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
  tcp->init();
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
  tcp->init();
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

CASE("TCP header flags")
{
  auto tcp = create_tcp_packet();
  tcp->init();
  using namespace net::tcp;

  EXPECT(tcp->isset(SYN) == false);
  tcp->set_flags(SYN);
  EXPECT(tcp->isset(SYN) == true);

  EXPECT(tcp->isset(FIN) == false);
  tcp->set_flags(FIN);
  EXPECT(tcp->isset(SYN) == true);
  EXPECT(tcp->isset(FIN) == true);

  tcp->clear_flags();
  EXPECT(tcp->isset(SYN) == false);
  EXPECT(tcp->isset(FIN) == false);

  tcp->set_flags(SYN | FIN);
  EXPECT(tcp->isset(SYN) == true);
  EXPECT(tcp->isset(FIN) == true);

  tcp->clear_flag(SYN);
  EXPECT(tcp->isset(SYN) == false);
  EXPECT(tcp->isset(FIN) == true);
}

CASE("TCP header source and dest")
{
  auto tcp = create_tcp_packet();
  tcp->init();

  tcp->set_source({ip4::Addr{10,0,0,1}, 666});
  tcp->set_destination({ip4::Addr{10,0,0,2}, 667});

  EXPECT(tcp->source().address() == ip4::Addr(10,0,0,1));
  EXPECT(tcp->source().port()    == 666);
  EXPECT(tcp->destination().address() == ip4::Addr(10,0,0,2));
  EXPECT(tcp->destination().port()    == 667);
}

CASE("TCP checksum")
{
  auto tcp = create_tcp_packet();
  tcp->init();

  tcp->set_source({ip4::Addr{10,0,0,1}, 666});
  tcp->set_destination({ip4::Addr{10,0,0,2}, 667});

  tcp->set_tcp_checksum();
  EXPECT(tcp->compute_tcp_checksum() == 0);

  tcp->set_src_port(322);
  EXPECT(tcp->src_port() == 322);
  EXPECT(tcp->compute_tcp_checksum() != 0);

  tcp->set_tcp_checksum();
  EXPECT(tcp->compute_tcp_checksum() == 0);
}
