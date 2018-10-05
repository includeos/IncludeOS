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

#include <os>
#include <kernel/events.hpp>
#include <drivers/usernet.hpp>
#include "macfuzzy.hpp"

static fuzzy::AsyncDevice dev1;
static fuzzy::AsyncDevice dev2;
static const int TCP_PORT = 12345;

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
  
  printf(R"FUzzY(
   __  __                     ___                            _  _
  |  \/  |  __ _     __      | __|  _  _      ___     ___   | || |
  | |\/| | / _` |   / _|     | _|  | +| |    |_ /    |_ /    \_, |
  |_|__|_| \__,_|   \__|_   _|_|_   \_,_|   _/__|   _/__|   _|__/
  _|"""""|_|"""""|_|"""""|_| """ |_|"""""|_|"""""|_|"""""|_| """"|
  "`-0-0-'"`-0-0-'"`-0-0-'"`-0-0-'"`-0-0-'"`-0-0-'"`-0-0-'"`-0-0-' )FUzzY");
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
  
  // IP-stack fuzzing
  fuzzy::stack_config config {
    .layer = fuzzy::IP4,
    .ip_port = 0
  };
  fuzzy::insert_into_stack(dev1, config, data, size);
  // Upper layer fuzzing using fuzzy::Stream
  
  return 0;
}
