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

#include <cmath> // rand()
#include <os>
#include <hw/devices.hpp>
#include <kernel/events.hpp>
#include <drivers/usernet.hpp>
#include <net/inet>
#include "../router/async_device.hpp"

static std::unique_ptr<Async_device> dev1;
static std::unique_ptr<Async_device> dev2;
static std::vector<uint8_t> load_file(const std::string& file);
#define htons(x) __builtin_bswap16(x)
static const int TCP_PORT = 12345;

void Service::start()
{
  dev1 = std::make_unique<Async_device>(4000);
  dev2 = std::make_unique<Async_device>(4000);
  //dev1->connect(*dev2);
  //dev2->connect(*dev1);
  dev1->set_transmit(
    [] (net::Packet_ptr packet) {
      (void) packet;
      //fprintf(stderr, "."); // drop
    });

  auto& inet_server = net::Super_stack::get(0);
  inet_server.network_config({10,0,0,42}, {255,255,255,0}, {10,0,0,1});
  auto& inet_client = net::Super_stack::get(1);
  inet_client.network_config({10,0,0,43}, {255,255,255,0}, {10,0,0,1});
  
#ifndef LIBFUZZER_ENABLED
  std::vector<std::string> files = {
  };
  for (const auto& file : files) {
    auto v = load_file(file);
    printf("*** Inserting payload %s into stack...\n", file.c_str());
    insert_into_stack(v.data(), v.size());
    printf("*** Payload %s was inserted into stack\n", file.c_str());
    printf("\n");
  }
#endif

  inet_server.resolve("www.oh.no",
      [] (net::IP4::addr addr, const auto& error) -> void {
        (void) addr;
        (void) error;
        printf("resolve() call ended\n");
      });
  inet_server.tcp().listen(
    TCP_PORT,
    [] (auto connection) {
      printf("Writing test to new connection\n");
      connection->write("test");
    });
}

enum layer_t {
  ETH,
  IP4,
  TCP,
  UDP,
  DNS,
  TCP_CONNECTION
};

#include "fuzzy_helpers.hpp"

static uint8_t*
add_ip4_layer(uint8_t* data, FuzzyIterator& fuzzer,
              const net::ip4::Addr src_addr,
              const net::ip4::Addr dst_addr,
              const uint8_t protocol = 0)
{
  auto* hdr = new (data) net::ip4::Header();
  hdr->ttl      = 64;
  hdr->protocol = (protocol) ? protocol : fuzzer.steal8();
  hdr->check    = 0;
  hdr->tot_len  = htons(sizeof(net::ip4::Header) + fuzzer.size);
  hdr->saddr    = src_addr;
  hdr->daddr    = dst_addr;
  //hdr->check    = net::checksum(hdr, sizeof(net::ip4::header));
  fuzzer.increment_data(sizeof(net::ip4::Header));
  return &data[sizeof(net::ip4::Header)];
}
static uint8_t*
add_udp4_layer(uint8_t* data, FuzzyIterator& fuzzer,
              const uint16_t dport)
{
  auto* hdr = new (data) net::UDP::header();
  hdr->sport = htons(fuzzer.steal16());
  hdr->dport = htons(dport);
  hdr->length = htons(fuzzer.size);
  hdr->checksum = 0;
  fuzzer.increment_data(sizeof(net::UDP::header));
  return &data[sizeof(net::UDP::header)];
}
static uint8_t*
add_tcp4_layer(uint8_t* data, FuzzyIterator& fuzzer,
              const uint16_t dport)
{
  auto* hdr = new (data) net::tcp::Header();
  hdr->source_port      = htons(1234);
  hdr->destination_port = htons(dport);
  hdr->seq_nr      = fuzzer.steal32();
  hdr->ack_nr      = fuzzer.steal32();
  hdr->offset_flags.offset_reserved = 0;
  hdr->offset_flags.flags = fuzzer.steal8();
  hdr->window_size = fuzzer.steal16();
  hdr->checksum    = 0;
  hdr->urgent      = 0;
  fuzzer.increment_data(sizeof(net::tcp::Header));
  return &data[sizeof(net::tcp::Header)];
}
static uint8_t*
add_eth_layer(uint8_t* data, FuzzyIterator& fuzzer, net::Ethertype type)
{
  auto* eth = (net::ethernet::Header*) data;
  eth->set_src({0x1, 0x2, 0x3, 0x4, 0x5, 0x6});
  eth->set_dest({0x7, 0x8, 0x9, 0xA, 0xB, 0xC});
  eth->set_type(type);
  fuzzer.increment_data(sizeof(net::ethernet::Header));
  return &data[sizeof(net::ethernet::Header)];
}

static inline uint16_t udp_port_scan(net::Inet& inet)
{
  for (uint16_t udp_port = 1; udp_port < 65535; udp_port++) {
    if (inet.udp().is_bound({inet.ip_addr(), udp_port})) {
      return udp_port;
    }
  }
  return 0;
}

static void
insert_into_stack(layer_t layer, const uint8_t* data, const size_t size)
{
  auto& inet = net::Super_stack::get(0);
  const size_t packet_size = std::min((size_t) inet.MTU(), size);
  FuzzyIterator fuzzer{data, packet_size};
  
  auto p = inet.create_packet();
  // link layer -> IP4
  auto* eth_end = add_eth_layer(p->layer_begin(), fuzzer,
                                net::Ethertype::IP4);
  // select layer to fuzz
  switch (layer) {
  case ETH:
    // by subtracting 2 i can fuzz ethertype as well
    fuzzer.fill_remaining(eth_end);
    break;
  case IP4:
    {
      auto* next_layer = add_ip4_layer(eth_end, fuzzer,
                         {10, 0, 0, 1}, inet.ip_addr());
      fuzzer.fill_remaining(next_layer);
      break;
    }
  case UDP:
    {
      // scan for UDP port (once)
      static uint16_t udp_port = 0;
      if (udp_port == 0) {
        udp_port = udp_port_scan(inet);
        assert(udp_port != 0);
      }
      // generate IP4 and UDP datagrams
      auto* ip_layer = add_ip4_layer(eth_end, fuzzer,
                         {10, 0, 0, 1}, inet.ip_addr(),
                          (uint8_t) net::Protocol::UDP);
      auto* udp_layer = add_udp4_layer(ip_layer, fuzzer,
                          udp_port);
      fuzzer.fill_remaining(udp_layer);
      break;
    }
  case TCP:
    {
      // generate IP4 and TCP data
      auto* ip_layer = add_ip4_layer(eth_end, fuzzer,
                         {10, 0, 0, 1}, inet.ip_addr(),
                          (uint8_t) net::Protocol::TCP);
      auto* tcp_layer = add_tcp4_layer(ip_layer, fuzzer,
                          TCP_PORT);
      fuzzer.fill_remaining(tcp_layer);
      break;
    }
  case TCP_CONNECTION:
    //
    break;
  default:
    assert(0 && "Implement me");
  }
  // we have to add ethernet size here as its not part of MTU
  p->set_data_end(fuzzer.data_counter);
  dev1->get_driver()->receive(std::move(p));
}

std::vector<uint8_t> load_file(const std::string& file)
{
	FILE *f = fopen(file.c_str(), "rb");
	if (f == nullptr) return {};
	fseek(f, 0, SEEK_END);
	const size_t size = ftell(f);
	fseek(f, 0, SEEK_SET);
  std::vector<uint8_t> data(size);
	if (size != fread(data.data(), sizeof(char), size, f)) {
		return {};
	}
	fclose(f);
	return data;
}

// libfuzzer input
extern "C"
int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
  static bool init_once = false;
  if (UNLIKELY(init_once == false)) {
    init_once = true;
    extern int userspace_main(int, const char*[]);
    const char* args[] = {"test"};
    userspace_main(1, args);
  }
  if (size == 0) return 0;
  insert_into_stack(UDP, data, size);
  return 0;
}
