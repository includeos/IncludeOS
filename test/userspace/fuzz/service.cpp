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
#include "macfuzzy.hpp"

static fuzzy::AsyncDevice_ptr dev1;
static fuzzy::AsyncDevice_ptr dev2;
static const int TCP_PORT = 12345;
static uint16_t  TCP_LOCAL_PORT = 0;
static int  TCP_buflen = 0;
static char TCP_buffer[8192] __attribute__((aligned(16)));

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

#include <net/interfaces>
void Service::start()
{
  dev1 = std::make_unique<fuzzy::AsyncDevice>(UserNet::create(4000));
  dev2 = std::make_unique<fuzzy::AsyncDevice>(UserNet::create(4000));
	dev1->connect(*dev2);
	dev2->connect(*dev1);

  auto& inet_server = net::Interfaces::get(0);
  inet_server.network_config({10,0,0,42}, {255,255,255,0}, {10,0,0,1});
  auto& inet_client = net::Interfaces::get(1);
  inet_client.network_config({10,0,0,43}, {255,255,255,0}, {10,0,0,1});

	const auto mode = std::string(getenv("FUZZ"));

	if (mode == "DNS" || mode == "UDP")
	{
	  inet_server.resolve("www.oh.no",
	      [] (net::dns::Response_ptr resp, const net::Error& error) -> void {
	        (void) resp;
	        (void) error;
	        printf("!!\n!! resolve() call ended\n!!\n");
	      });
	}
	else if (mode == "TCP")
	{
	  inet_server.tcp().listen(
	    TCP_PORT,
	    [] (auto connection) {
	      //connection->write("test");
	    });

		static bool connected = false;
		auto conn = inet_client.tcp().connect({inet_server.ip_addr(), TCP_PORT});
		conn->on_connect(
	    [] (auto connection) {
				connected = true;
	    });
		// block until connected
		while (!connected) {
			Events::get().process_events();
		}
		TCP_LOCAL_PORT = conn->local().port();
		printf("TCP source port is %u\n", TCP_LOCAL_PORT);
		TCP_buflen = conn->serialize_to(TCP_buffer);
		conn->abort();
	}

	// make sure error packets are discarded
	dev1->set_transmit(
    [] (net::Packet_ptr packet) {
      (void) packet;
      //fprintf(stderr, "."); // drop
    });

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
#else
  printf(R"FUzzY(
   __  __                     ___                            _  _
  |  \/  |  __ _     __      | __|  _  _      ___     ___   | || |
  | |\/| | / _` |   / _|     | _|  | +| |    |_ /    |_ /    \_, |
  |_|__|_| \__,_|   \__|_   _|_|_   \_,_|   _/__|   _/__|   _|__/
  _|"""""|_|"""""|_|"""""|_| """ |_|"""""|_|"""""|_|"""""|_| """"|
  "`-0-0-'"`-0-0-'"`-0-0-'"`-0-0-'"`-0-0-'"`-0-0-'"`-0-0-'"`-0-0-'
)FUzzY");
#endif
}

#include "fuzzy_http.hpp"
#include "fuzzy_stream.hpp"
//#include "fuzzy_webserver.cpp"
extern net::tcp::Connection_ptr deserialize_connection(void* addr, net::TCP& tcp);

struct serialized_tcp
{
  typedef net::tcp::Connection Connection;
  typedef net::tcp::port_t     port_t;
  typedef net::Socket          Socket;

  Socket local;
  Socket remote;

  int8_t  state_now;
  int8_t  state_prev;

  Connection::TCB tcb;
} __attribute__((packed));
static inline uint32_t extract_seq() {
	return ((serialized_tcp*) TCP_buffer)->tcb.RCV.NXT;
}
static inline uint32_t extract_ack() {
	return ((serialized_tcp*) TCP_buffer)->tcb.SND.NXT;
}

static void test_tcp(const uint8_t* data, size_t size)
{
	auto& inet = net::Interfaces::get(0);
	// create connection
	auto conn = deserialize_connection(TCP_buffer, inet.tcp());
	//printf("Deserialized %s\n", conn->to_string().c_str());

  // IP-stack fuzzing
  const fuzzy::stack_config config {
    .layer   = fuzzy::TCP,
    .ip_port = TCP_PORT,
		.ip_src_port = TCP_LOCAL_PORT,
		.tcp_seq = extract_seq(),
		.tcp_ack = extract_ack()
  };
	insert_into_stack(dev1, config, data, size);

	conn->abort();
}

static void test_udp(const uint8_t* data, size_t size)
{
  const fuzzy::stack_config config {
    .layer   = fuzzy::UDP,
    .ip_port = 0
  };
	insert_into_stack(dev1, config, data, size);
}

static void test_eth(const uint8_t* data, size_t size)
{
  const fuzzy::stack_config config {
    .layer   = fuzzy::ETH,
  };
	insert_into_stack(dev1, config, data, size);
}
static void test_ip4(const uint8_t* data, size_t size)
{
  const fuzzy::stack_config config {
    .layer   = fuzzy::IP4,
  };
	insert_into_stack(dev1, config, data, size);
}
static void test_ip6(const uint8_t* data, size_t size)
{
  const fuzzy::stack_config config {
    .layer   = fuzzy::IP6,
  };
	insert_into_stack(dev1, config, data, size);
}
static void test_icmp6(const uint8_t* data, size_t size)
{
  const fuzzy::stack_config config {
    .layer   = fuzzy::ICMP6,
  };
	insert_into_stack(dev1, config, data, size);
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

	const auto mode = std::string(getenv("FUZZ"));
	if (mode == "TCP") {
		test_tcp(data, size);
	}
	else if (mode == "UDP") {
		test_udp(data, size);
	}
	else if (mode == "ETH") {
		test_eth(data, size);
	}
	else if (mode == "IP4") {
		test_ip4(data, size);
	}
	else if (mode == "IP6") {
		test_ip6(data, size);
	}
	else if (mode == "ICMP6") {
		test_icmp6(data, size);
	}
	else if (mode == "HTTP") {
		fuzzy_http(data, size);
	}
	else if (mode == "WEBSOCKET") {
		fuzzy_websocket(data, size);
	}
	else {
		assert(0 && "No such fuzzing target");
	}

  return 0;
}
