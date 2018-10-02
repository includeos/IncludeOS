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
static void insert_into_stack(const uint8_t* data, const size_t size);
static std::vector<uint8_t> load_file(const std::string& file);

void Service::start()
{
  dev1 = std::make_unique<Async_device>(4000);
  dev2 = std::make_unique<Async_device>(4000);
  //dev1->connect(*dev2);
  //dev2->connect(*dev1);
  dev1->set_transmit(
    [] (net::Packet_ptr packet) {
      // drop
      fprintf(stderr, "Dropping outgoing packet\n");
    });

  auto& inet_server = net::Super_stack::get(0);
  inet_server.network_config({10,0,0,42}, {255,255,255,0}, {10,0,0,1});
  auto& inet_client = net::Super_stack::get(1);
  inet_client.network_config({10,0,0,43}, {255,255,255,0}, {10,0,0,1});
  
#ifndef LIBFUZZER_ENABLED
  std::vector<std::string> files = {
    "crash-ac14c3b630c779e70d5e47363ca5d48842c9704f"
  };
  for (const auto& file : files) {
    auto v = load_file(file);
    printf("*** Inserting payload %s into stack...\n", file.c_str());
    insert_into_stack(v.data(), v.size());
    printf("*** Payload %s was inserted into stack\n", file.c_str());
    printf("\n");
  }
#endif
}

void insert_into_stack(const uint8_t* data, const size_t size)
{
  auto& inet = net::Super_stack::get(0);
  auto p = inet.create_packet();
  auto* eth = (net::ethernet::Header*) p->buf();
  eth->set_src({0x1, 0x2, 0x3, 0x4, 0x5, 0x6});
  eth->set_dest({0x7, 0x8, 0x9, 0xA, 0xB, 0xC});
  eth->set_type(net::Ethertype::IP4);
  const size_t packet_size = std::min((size_t) inet.MTU(), size);
  // we have to add ethernet size here as its not part of MTU
  p->set_data_end(sizeof(net::ethernet::Header) + packet_size);
  // by subtracting 2 i can fuzz ethertype as well
  memcpy(eth->next_layer, data, packet_size);
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
  insert_into_stack(data, size);
  return 0;
}
