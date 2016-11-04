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
#include <net/inet4>
#include <net/dhcp/dh4client.hpp>
#include <net/tcp/tcp.hpp>
#include <vector>
#include <info>
#include <timers>

using namespace net;
using namespace std::chrono; // For timers and MSL
tcp::Connection_ptr client;

/*
  TEST VARIABLES
*/
tcp::port_t
TEST1{8081}, TEST2{8082}, TEST3{8083}, TEST4{8084}, TEST5{8085};

using HostAddress = std::pair<std::string, tcp::port_t>;
HostAddress
TEST_ADDR_TIME{"india.colorado.edu", 13};

std::string
small, big, huge;

int
S{150}, B{1500}, H{150000};

std::string
TEST_STR {"Kappa!"};

size_t buffers_available{0};

// Default MSL is 30s. Timeout 2*MSL.
// To reduce test duration, lower MSL to 3s.
milliseconds MSL_TEST = 3s;

/*
  TEST: Release of resources/clean up.
*/
void FINISH_TEST() {
  INFO("TEST", "Started 2 x MSL + 100ms timeout.");
  Timers::oneshot(2 * MSL_TEST + 100ms,
  [] (Timers::id_t) {
      INFO("TEST", "Verify release of resources");
      CHECKSERT(Inet4::stack<0>().tcp().active_connections() == 0,
        "No (0) active connections");
      INFO("Buffers available", "%u", Inet4::stack<0>().buffers_available());
      CHECKSERT(Inet4::stack<0>().buffers_available() == buffers_available,
        "No hogged buffer (%u available)", buffers_available);
      printf("# TEST SUCCESS #\n");
    });
}

/*
  TEST: Outgoing Internet Connection
*/
void OUTGOING_TEST_INTERNET(const HostAddress& address) {
  auto port = address.second;
  // This needs correct setup to work
  INFO("TEST", "Outgoing Internet Connection (%s:%u)", address.first.c_str(), address.second);
  Inet4::stack<0>().resolve(address.first,
    [port](auto ip_address) {
      CHECK(ip_address != 0, "Resolved host");

      if(ip_address != 0) {
        Inet4::stack<0>().tcp().connect(ip_address, port)
          ->on_connect([](tcp::Connection_ptr conn) {
              CHECK(true, "Connected");
              conn->on_read(1024, [](tcp::buffer_t, size_t n) {
                  CHECK(n > 0, "Received a response");
                });
            })
          .on_error([](tcp::TCPException err) {
              CHECK(false, "Error occured: %s", err.what());
            });
      }
    });
}

/*
  TEST: Outgoing Connection to Host
*/
void OUTGOING_TEST(tcp::Socket outgoing) {
  INFO("TEST", "Outgoing Connection (%s)", outgoing.to_string().c_str());
  Inet4::stack<0>().tcp().connect(outgoing)
    ->on_connect([](auto conn) {
        conn->write(small.data(), small.size());
        conn->on_read(small.size(), [](tcp::buffer_t buffer, size_t n) {
            CHECKSERT(std::string((char*)buffer.get(), n) == small, "Received SMALL");
          });
      })
    .on_disconnect([](auto conn, tcp::Connection::Disconnect) {
        CHECK(true, "Connection closed by server");
        CHECKSERT(conn->is_state({"CLOSE-WAIT"}), "State: CLOSE-WAIT");
        conn->close();
      })
    .on_close([]{ OUTGOING_TEST_INTERNET(TEST_ADDR_TIME); });
}

// Used to send big data
struct Buffer {
  size_t written, read;
  char* data;
  const size_t size;

  Buffer(size_t length) :
    written(0), read(0), data(new char[length]), size(length) {}

  ~Buffer() { delete[] data; }

  std::string str() { return {data, size};}
};

void Service::start(const std::string&)
{
  IP4::addr A1 (255, 255, 255, 255);
  IP4::addr B1 (  0, 255, 255, 255);
  IP4::addr C1 (  0,   0, 255, 255);
  IP4::addr D1 (  0,   0,   0, 255);
  IP4::addr E1 (  0,   0,   0,   0);
  printf("A: %s\n", A1.str().c_str());
  printf("B: %s\n", B1.str().c_str());
  printf("C: %s\n", C1.str().c_str());
  printf("D: %s\n", D1.str().c_str());
  printf("E: %s\n", E1.str().c_str());
  printf("D & A: %s\n", (D1 & A1).str().c_str());

  for(int i = 0; i < S; i++) small += TEST_STR;

  big += "start-";
  for(int i = 0; i < B; i++) big += TEST_STR;
  big += "-end";

  huge = "start-";
  for(int i = 0; i < H; i++) huge += TEST_STR;
  huge += "-end";

  auto& inet = Inet4::stack<0>(); // Inet4<VirtioNet>::stack<0>();
  inet.network_config(
    {  10,  0,  0, 44 },  // IP
    {  255,255,255, 0 },  // Netmask
    {  10,  0,  0,  1 },  // Gateway
    {   8,  8,  8,  8 }   // DNS
  );

  buffers_available = inet.buffers_available();
  INFO("Buffers available", "%u", inet.buffers_available());

  auto& tcp = inet.tcp();
  // reduce test duration
  tcp.set_MSL(MSL_TEST);

  /*
    TEST: Send and receive small string.
  */
  INFO("TEST", "Listeners and connections allocation.");

  /*
    TEST: Nothing should be allocated.
  */
  CHECK(tcp.open_ports() == 0, "No (0) open ports (listening connections)");
  CHECK(tcp.active_connections() == 0, "No (0) active connections");

  tcp.bind(TEST1).on_connect([](auto conn) {
      INFO("TEST", "SMALL string (%u)", small.size());
      conn->on_read(small.size(), [conn](tcp::buffer_t buffer, size_t n) {
          CHECKSERT(std::string((char*)buffer.get(), n) == small, "Received SMALL");
          conn->close();
        });
      conn->write(small);
    });

  /*
    TEST: Server should be bound.
  */
  CHECK(tcp.open_ports() == 1, "One (1) open port");

  /*
    TEST: Send and receive big string.
  */
  tcp.bind(TEST2).on_connect([](auto conn) {
      INFO("TEST", "BIG string (%u)", big.size());
      auto response = std::make_shared<std::string>();
      conn->on_read(big.size(), [response, conn](tcp::buffer_t buffer, size_t n) {
          *response += std::string((char*)buffer.get(), n);
          if(response->size() == big.size()) {
            bool OK = (*response == big);
            CHECKSERT(OK, "Received BIG");
            conn->close();
          }
        });
      conn->write(big.data(), big.size());
    });

  /*
    TEST: Send and receive huge string.
  */
  tcp.bind(TEST3).on_connect([](auto conn) {
      INFO("TEST", "HUGE string (%u)", huge.size());
      auto temp = std::make_shared<Buffer>(huge.size());
      conn->on_read(16384, [temp, conn](tcp::buffer_t buffer, size_t n) {
          memcpy(temp->data + temp->written, buffer.get(), n);
          temp->written += n;
          //printf("Read: %u\n", n);
          // when all expected data is read
          if(temp->written == huge.size()) {
            bool OK = (temp->str() == huge);
            CHECKSERT(OK, "Received HUGE");
            conn->close();
          }
        });
      auto half = huge.size() / 2;
      conn->write(huge.data(), half, [half, conn](size_t n) {
        CHECKSERT(n == half, "Wrote one half HUGE (%u bytes)", n);
      });
      conn->write(huge.data()+half, half, [half](size_t n) {
        CHECKSERT(n == half, "Wrote the other half of HUGE (%u bytes)", n);
      });
    });

  /*
    TEST: More servers should be bound.
  */
  CHECK(tcp.open_ports() == 3, "Three (3) open ports");

  /*
    TEST: Connection (Status etc.) and Active Close
  */
  tcp.bind(TEST4).on_connect([](auto conn) {
      INFO("TEST","Connection/TCP state");
      // There should be at least one connection.
      CHECKSERT(Inet4::stack<0>().tcp().active_connections() > 0, "There is (>0) open connection(s)");
      // Test if connected.
      CHECKSERT(conn->is_connected(), "Is connected");
      // Test if writable.
      CHECKSERT(conn->is_writable(), "Is writable");
      // Test if state is ESTABLISHED.
      CHECKSERT(conn->is_state({"ESTABLISHED"}), "State: ESTABLISHED");

      INFO("TEST", "Active close");
      CHECKSERT(!conn->is_closing(), "Is NOT closing");
      // Setup on_disconnect event
      conn->on_disconnect([](auto conn, tcp::Connection::Disconnect) {
        CHECKSERT(conn->is_closing(), "Is closing");
        CHECKSERT(conn->is_state({"FIN-WAIT-2"}), "State: FIN-WAIT-2");
        Timers::oneshot(1s,
        [conn] (auto) {
            CHECKSERT(conn->is_state({"TIME-WAIT"}), "State: TIME-WAIT");

            OUTGOING_TEST({Inet4::stack().gateway(), TEST5});
          });

        Timers::oneshot(5s, [] (Timers::id_t) { FINISH_TEST(); });
      });

      // Test for active close.
      conn->close();
      CHECKSERT(!conn->is_writable(), "Is NOT writable");
      CHECKSERT(conn->is_state({"FIN-WAIT-1"}), "State: FIN-WAIT-1");
    });

printf ("IncludeOS TCP test\n");
}
