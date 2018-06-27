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
#include <net/inet.hpp>
#include <net/super_stack.hpp>
#include <vector>
#include <info>
#include <timers>

using namespace net;
using namespace std::chrono; // For timers and MSL
tcp::Connection_ptr client;

static Inet& stack()
{ return Super_stack::get(0); }

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
      CHECKSERT(stack().tcp().active_connections() == 0,
        "No (0) active connections");
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
  stack().resolve(address.first,
    [port](auto ip_address, const Error&) {
      CHECK(ip_address != 0, "Resolved host");

      if(ip_address != 0)
      {
        stack().tcp().connect({ip_address, port})
          ->on_connect([](tcp::Connection_ptr conn)
          {
            CHECKSERT(conn != nullptr, "Connected");
            conn->on_read(1024,
              [](tcp::buffer_t buf) {
                CHECK(buf->size() > 0, "Received a response");
              });
          });
      }
    });
}

/*
  TEST: Outgoing Connection to Host
*/
void OUTGOING_TEST(Socket outgoing) {
  INFO("TEST", "Outgoing Connection (%s)", outgoing.to_string().c_str());
  stack().tcp().connect(outgoing, [](tcp::Connection_ptr conn)
  {
    CHECKSERT(conn != nullptr, "Connection successfully established.");
    conn->on_read(small.size(),
    [](tcp::buffer_t buffer)
    {
      const std::string str((char*)buffer->data(), buffer->size());
      CHECKSERT(str == small, "Received SMALL");
    });

    conn->write(small);

    conn->on_disconnect([](tcp::Connection_ptr conn, tcp::Connection::Disconnect) {
      CHECK(true, "Connection closed by server");
      CHECKSERT(conn->is_state({"CLOSE-WAIT"}), "State: CLOSE-WAIT");
      conn->close();
    })
    .on_close([]{ OUTGOING_TEST_INTERNET(TEST_ADDR_TIME); });
  });
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

void Service::start()
{
#ifdef USERSPACE_LINUX
  extern void create_network_device(int N, const char* route, const char* ip);
  create_network_device(0, "10.0.0.0/24", "10.0.0.1");
#endif

  for(int i = 0; i < S; i++) small += TEST_STR;

  big += "start-";
  for(int i = 0; i < B; i++) big += std::to_string(i) + "-";
  big += "-end";

  huge = "start-";
  for(int i = 0; i < H; i++) huge += TEST_STR;
  huge += "-end";

  auto& inet = stack();
  inet.network_config(
    {  10,  0,  0, 44 },  // IP
    {  255,255,255, 0 },  // Netmask
    {  10,  0,  0,  1 },  // Gateway
    {   8,  8,  8,  8 }   // DNS
  );
  inet.network_config6(
    {  0xfe80, 0, 0, 0, 0xe823, 0xfcff, 0xfef4, 0x85bd },   // IP6
    64,                                                     // Prefix6
    {  0xfe80,  0,  0, 0, 0xe823, 0xfcff, 0xfef4, 0x83e7 }  // Gateway6
  );

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
  CHECK(tcp.listening_ports() == 0, "No (0) open ports (listening connections)");
  CHECK(tcp.active_connections() == 0, "No (0) active connections");

  tcp.listen(TEST1).on_connect([](tcp::Connection_ptr conn) {
      INFO("Test 1", "SMALL string (%u)", small.size());
      conn->on_read(small.size(), [conn](tcp::buffer_t buffer) {
          CHECKSERT(std::string((char*)buffer->data(), buffer->size()) == small, "Received SMALL");
          INFO("Test 1", "Succeeded, TEST2");
          conn->close();
        });
      conn->write(small);
    });

  /*
    TEST: Server should be bound.
  */
  CHECK(tcp.listening_ports() == 1, "One (1) open port");

  /*
    TEST: Send and receive big string.
  */
  tcp.listen(TEST2).on_connect([](tcp::Connection_ptr conn) {
      INFO("Test 2", "BIG string (%u)", big.size());
      auto response = std::make_shared<std::string>();
      conn->on_read(big.size(),
      [response, conn] (tcp::buffer_t buffer)
        {
          response->append(std::string{(char*)buffer->data(), buffer->size()});
          if(response->size() == big.size()) {
            CHECKSERT((*response == big), "Received BIG");
            INFO("Test 2", "Succeeded, TEST3");
            conn->close();
          }
        });
      conn->write(big.data(), big.size());
    });

  /*
    TEST: Send and receive huge string.
  */
  tcp.listen(TEST3).on_connect([](tcp::Connection_ptr conn) {
      INFO("Test 3", "HUGE string (%u)", huge.size());
      auto temp = std::make_shared<Buffer>(huge.size());
      conn->on_read(16384,
        [temp, conn] (tcp::buffer_t buffer) {
          memcpy(temp->data + temp->written, buffer->data(), buffer->size());
          temp->written += buffer->size();
          // when all expected data is read
          if(temp->written == huge.size()) {
            bool OK = (temp->str() == huge);
            CHECKSERT(OK, "Received HUGE");
            INFO("Test 3", "Succeeded, TEST4");
          }
        });
      auto half = huge.size() / 2;
      conn->on_write([half](size_t n) {
        CHECKSERT(n == half, "Wrote half HUGE (%lu bytes)", n);
      });
      conn->write(huge.data(), half);
      conn->write(huge.data()+half, half);
      conn->close();
    });

  /*
    TEST: More servers should be bound.
  */
  CHECK(tcp.listening_ports() == 3, "Three (3) open ports");

  /*
    TEST: Connection (Status etc.) and Active Close
  */
  tcp.listen(TEST4).on_connect([](tcp::Connection_ptr conn) {
      INFO("Test 4","Connection/TCP state");
      // There should be at least one connection.
      CHECKSERT(stack().tcp().active_connections() > 0, "There is (>0) open connection(s)");
      // Test if connected.
      CHECKSERT(conn->is_connected(), "Is connected");
      // Test if writable.
      CHECKSERT(conn->is_writable(), "Is writable");
      // Test if state is ESTABLISHED.
      CHECKSERT(conn->is_state({"ESTABLISHED"}), "State: ESTABLISHED");

      INFO("Test 4", "Active close");
      CHECKSERT(!conn->is_closing(), "Is NOT closing");
      // Setup on_disconnect event
      conn->on_disconnect([](auto conn, tcp::Connection::Disconnect) {
        CHECKSERT(conn->is_closing(), "Is closing");
        CHECKSERT(conn->is_state({"FIN-WAIT-2"}), "State: FIN-WAIT-2");
        Timers::oneshot(1s,
        [conn] (auto) {
            CHECKSERT(conn->is_state({"TIME-WAIT"}), "State: TIME-WAIT");
            INFO("Test 4", "Succeeded. Trigger TEST5");
            OUTGOING_TEST({Inet::stack().gateway6(), TEST5});
          });

        Timers::oneshot(5s, [] (Timers::id_t) { FINISH_TEST(); });
      });

      // Test for active close.
      conn->close();
      CHECKSERT(!conn->is_writable(), "Is NOT writable");
      CHECKSERT(conn->is_state({"FIN-WAIT-1"}), "State: FIN-WAIT-1");
    });

  printf ("IncludeOS TCP test\n");
  printf("TEST1 \n");

}
