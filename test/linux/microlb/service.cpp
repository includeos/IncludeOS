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
#include <microLB>
#include <net/tcp/stream.hpp>
#include <hw/async_device.hpp>
static microLB::Balancer* balancer = nullptr;
static std::unique_ptr<hw::Async_device<UserNet>> dev1 = nullptr;
static std::unique_ptr<hw::Async_device<UserNet>> dev2 = nullptr;

static const int NUM_ROUNDS = 4;
static const int NUM_STAGES = 4;
static int test_rounds_completed = 0;
static bool are_all_streams_at_stage(int stage);
static void do_test_again();
static void do_test_completed();
static const std::string long_string(256000, '-');

static bool generic_on_read(net::Stream::buffer_t buffer, std::string& read_buffer)
{
  read_buffer += std::string(buffer->begin(), buffer->end());
  if (read_buffer == "Hello!") return true;
  else if (read_buffer == "Second write") return true;
  else if (read_buffer == long_string) return true;
  // else: ... wait for more data
  return false;
}

struct Client
{
  net::tcp::Connection_ptr conn;
  std::string read_buffer = "";
  int  idx = 0;
  int  test_stage = 0;
  const char* token() const { return "CLIENT"; }

  Client(net::tcp::Connection_ptr c)
      : conn(c)
  {
    static int cli_counter = 0;
    this->idx = cli_counter++;
    this->conn->on_close({this, &Client::on_close});
    //this->conn->on_read(6666, {this, &Client::on_read});
    printf("%s %d created\n", token(), this->idx);
    // send data immediately?
    this->send_data();
  }

  void send_data()
  {
    conn->write("Hello!");
    conn->write("Second write");
    conn->write(long_string);
  }
  void on_read(net::Stream::buffer_t buffer)
  {
    printf("%s %d on_read: %zu bytes\n", token(), this->idx, buffer->size());
    if (generic_on_read(buffer, read_buffer)) {
      this->test_stage_advance();
    }
  }
  void on_close()
  {
    printf("%s %d on_close called\n", token(), this->idx);
    this->test_stage_advance();
  }
  void test_stage_advance()
  {
    this->test_stage ++;
    this->read_buffer.clear();
    if (this->test_stage == NUM_STAGES) {
      printf("[%s %d] Test stage: %d / %d\n",
             token(), this->idx, this->test_stage, NUM_STAGES);
    }
    
    if (are_all_streams_at_stage(NUM_STAGES)) {
      if (++test_rounds_completed == NUM_ROUNDS) {
        do_test_completed(); return;
      }
      do_test_again();
    }
  }
};
static std::list<Client> clients;

bool are_all_streams_at_stage(int stage)
{
  for (auto& client : clients) {
    if (client.test_stage != stage) return false;
  }
  return true;
}
void do_test_completed()
{
  printf("SUCCESS\n");
  OS::shutdown();
}

static void create_delayed_client()
{
  Timers::oneshot(1ms,
    [] (int) {
      auto& inet_server = net::Interfaces::get(0);
      auto& inet_client = net::Interfaces::get(1);
      auto conn = inet_server.tcp().connect({inet_client.ip_addr(), 666});
      conn->on_connect(
        [] (net::tcp::Connection_ptr conn) {
          assert(conn != nullptr);
          clients.push_back(conn);
        });
    });
}

void do_test_again()
{
  printf("Doing tests again\n");
  clients.erase(std::remove_if(
        clients.begin(), clients.end(),
        [] (Client& c) {
          printf("Erasing %s %d\n", c.token(), c.idx);
          return c.conn->is_closed();
        }), clients.end());
  // create new clients
  for (int i = 0; i < 10; i++)
  {
    create_delayed_client();
  }
  printf("Test starting now\n");
}

// the application
static void application_connection(net::tcp::Connection_ptr conn)
{
  assert(conn != nullptr);
  conn->on_read(8888,
    [conn] (net::tcp::buffer_t buffer) {
      // send buffer back as response
      conn->write(buffer);
      // and then close
      conn->close();
    });
}

// the configuration
static void setup_networks()
{
  dev1 = std::make_unique<hw::Async_device<UserNet>>(UserNet::create(1500));
  dev2 = std::make_unique<hw::Async_device<UserNet>>(UserNet::create(1500));
  dev1->connect(*dev2);
  dev2->connect(*dev1);

  // Create IP stacks on top of the nic's and configure them
  auto& inet_server = net::Interfaces::get(0);
  auto& inet_client = net::Interfaces::get(1);
  inet_server.network_config({10,0,0,42}, {255,255,255,0}, {10,0,0,1});
  inet_client.network_config({10,0,0,43}, {255,255,255,0}, {10,0,0,1});
}

// the load balancer
void Service::start()
{
  setup_networks();
  balancer = new microLB::Balancer();
  // application nodes on server interface
  auto& inet_server = net::Interfaces::get(0);
  auto& app = inet_server.tcp().listen(6667);
  app.on_connect(
  [] (net::tcp::Connection_ptr conn) {
      application_connection(conn);
  });
  // open for TCP connections on client interface
  auto& inet_client = net::Interfaces::get(1);
  balancer->open_for_tcp(inet_client, 666);
  
  // add a regular TCP node
  balancer->nodes.add_node(
        microLB::Balancer::connect_with_tcp(inet_client, {{10,0,0,42}, 6667}),
        balancer->get_pool_signal());
  
  do_test_again();
}
