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

#include <service>
#include <cstdio>
#include <net/inet4>
#include <timers>
#include "update.hpp"

struct HW_timer
{
  HW_timer(const std::string& str) {
    context = str;
    printf("HW timer starting for %s\n", context.c_str());
    time    = hw::CPU::rdtsc();
  }
  ~HW_timer() {
    auto diff = hw::CPU::rdtsc() - time;

    using namespace std::chrono;
    double  div  = OS::cpu_freq().count() * 1000000.0;
    int64_t time = diff / div * 1000;

    printf("HW timer for %s: %lld (%lld ms)\n", context.c_str(), diff, time);
  }
private:
  std::string context;
  int64_t     time;
};

// prevent default serial out
//void default_stdout_handlers() {}
#include <hw/serial.hpp>

typedef net::tcp::Connection_ptr Connection_ptr;
Connection_ptr deserialize_connection(void* addr, net::TCP& tcp);
std::vector<Connection_ptr> saveme;

template <typename T>
void setup_terminal(T& inet)
{
  // mini terminal
  printf("Setting up terminal, since we have not updated yet\n");
  
  auto& term = inet.tcp().bind(6667);
  term.on_connect(
  [] (auto conn) {
    saveme.push_back(conn);
    // write a string to change the state
    conn->write("State change SeemsGood\n");
    // retrieve binary
    conn->on_read(1024,
    [conn] (net::tcp::buffer_t buf, size_t n)
    {
      printf("Received message: %.*s\n", n, buf.get());
    });
  });
}

void Service::start(const std::string&)
{
  volatile HW_timer timer("Service::start()");
  // add own serial out after service start
  auto& com1 = hw::Serial::port<1>();
  OS::add_stdout(com1.get_print_handler());

  printf("Starting LiveUpdate test service\n\n");
  printf("CPU freq is %f MHz\n", OS::cpu_freq().count());

  auto& inet = net::Inet4::ifconfig<0>(
        { 10,0,0,42 },     // IP
        { 255,255,255,0 }, // Netmask
        { 10,0,0,1 },      // Gateway
        { 10,0,0,1 });     // DNS

  auto& server = inet.tcp().bind(666);
  server.on_connect(
  [] (auto conn)
  {
    static char* update_blob = new char[1024*1024*10];
    static int   update_size = 0;

    // reset update chunk
    update_size = 0;
    // retrieve binary
    conn->on_read(9000,
    [conn] (net::tcp::buffer_t buf, size_t n)
    {
      memcpy(update_blob + update_size, buf.get(), n);
      update_size += (int) n;

    }).on_close(
    [] {
      printf("* New update size: %u b\n", update_size);
      void save_stuff(Storage);
      LiveUpdate::begin({update_blob, update_size}, save_stuff);
      /// We should never return :-) ///
      assert(0 && "!! Update failed !!");
    });
  });

  if (!LiveUpdate::is_resumable())
      setup_terminal(inet);

  /// attempt to resume (if there is anything to resume)
  void the_string(Restore);
  void the_buffer(Restore);
  void the_timing(Restore);
  void restore_term(Restore);
  void on_missing(Restore);

  LiveUpdate::on_resume(1,   the_string);
  LiveUpdate::on_resume(2,   the_buffer);
  LiveUpdate::on_resume(100, the_timing);
  LiveUpdate::on_resume(666, restore_term);
  LiveUpdate::resume(on_missing);
  
  /*
  using namespace std::chrono;
  Timers::periodic(seconds(0), seconds(1),
  [] (auto) {
    printf("Time is %lld\n", hw::CPU::rdtsc());
  });*/
}

#include <hw/cpu.hpp>
void save_stuff(Storage storage)
{
  storage.add_string(1, "Some string :(");
  storage.add_string(1, "Some other string :(");

  char buffer[] = "Just some random buffer";
  storage.add_buffer(2, {buffer, sizeof(buffer)});

  auto ts = hw::CPU::rdtsc();
  storage.add_buffer(100, &ts, sizeof(ts));
  printf("! CPU ticks before: %lld\n", ts);

  for (auto conn : saveme)
      storage.add_connection(666, conn);
}

#include <hertz>
#include <chrono>
void the_string(Restore thing)
{
  printf("The string [some_string] has value [%s]\n", thing.as_string().c_str());
}
void the_buffer(Restore thing)
{
  printf("The buffer is %d long\n", thing.length());
  printf("As text: %.*s\n", thing.length(), thing.as_buffer().buffer);
}
void on_missing(Restore thing)
{
  printf("Missing resume function for %u\n", thing.get_id());
}

struct restore_t
{
  Connection_ptr conn = nullptr;
  char buffer[128];
  int  len = 0;
  
  void set_conn(Connection_ptr conn)
  {
    this->conn = conn;
    // also put this back in list of connections we store
    saveme.push_back(conn);
    
    printf("Restored terminal connection to %s\n", conn->remote().to_string().c_str());
    // restore callback
    conn->on_read(1024,
    [] (net::tcp::buffer_t buf, size_t n)
    {
      printf("Received message: %.*s\n", n, buf.get());
    });
    conn->on_close(
    [conn] {
      printf("Terminal %s closed\n", conn->to_string().c_str());
    });
  }
  
  void try_send()
  {
    if (conn != nullptr && len != 0)
    {
      printf("Sending message: %.*s\n", len, buffer);
      conn->write(buffer, len);
    }
  }
};
restore_t rest;

void the_timing(Restore thing)
{
  auto t1   = thing.as_type<int64_t>();
  auto diff = hw::CPU::rdtsc() - t1;

  using namespace std::chrono;
  double  div  = OS::cpu_freq().count() * 1000000.0;
  int64_t time = diff / div * 1000;

  rest.len = snprintf(rest.buffer, sizeof(rest.buffer),
             "! Boot time in ticks: %lld (%lld ms)\n", diff, time);

  printf("%.*s", rest.len, rest.buffer);
  rest.try_send();
}
void restore_term(Restore thing)
{
  auto& stack = net::Inet4::stack<0> ();
  rest.set_conn(thing.as_tcp_connection(stack.tcp()));
  // try sending the boot timing
  rest.try_send();
}
