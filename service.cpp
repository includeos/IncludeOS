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
#include <net/inet4>
#include <cstdio>
#include "update.hpp"
#include "hw_timer.hpp"

static const uint16_t TERM_PORT = 6667;

// prevent default serial out
void default_stdout_handlers() {}
#include <hw/serial.hpp>

typedef net::tcp::Connection_ptr Connection_ptr;
Connection_ptr deserialize_connection(void* addr, net::TCP& tcp);
static std::vector<Connection_ptr> saveme;
static std::vector<std::string>    savemsg;

void setup_terminal_connection(Connection_ptr conn)
{
  saveme.push_back(conn);
  // retrieve binary
  conn->on_read(512,
  [conn] (net::tcp::buffer_t buf, size_t n)
  {
    std::string str((const char*) buf.get(), n);
    printf("Received message %u: %s", savemsg.size()+1, str.c_str());
    // save for later as strings
    savemsg.push_back(str);
  });
  conn->on_close(
  [conn] {
    printf("Terminal %s closed\n", conn->to_string().c_str());
  });
}

template <typename T>
void setup_terminal(T& inet)
{
  // mini terminal
  printf("Setting up terminal on port %u\n", TERM_PORT);
  
  auto& term = inet.tcp().bind(TERM_PORT);
  term.on_connect(
  [] (auto conn) {
    setup_terminal_connection(conn);
    // write a string to change the state
    conn->write("State change SeemsGood\n");
  });
}

void Service::start(const std::string&)
{
  volatile HW_timer timer("Service::start()");
  // add own serial out after service start
  auto& com1 = hw::Serial::port<1>();
  OS::add_stdout(com1.get_print_handler());

  printf("\n");
  printf("-= Starting LiveUpdate test service =-\n");
  printf("* CPU freq is %f MHz\n", OS::cpu_freq().count());

  auto& inet = net::Inet4::ifconfig<0>(
        { 10,0,0,42 },     // IP
        { 255,255,255,0 }, // Netmask
        { 10,0,0,1 },      // Gateway
        { 10,0,0,1 });     // DNS

  /// attempt to resume (if there is anything to resume)
  void the_string(Restore);
  void the_buffer(Restore);
  void the_timing(Restore);
  void restore_term(Restore);
  void saved_message(Restore);
  void on_missing(Restore);

  LiveUpdate::on_resume(1,   the_string);
  LiveUpdate::on_resume(2,   the_buffer);
  LiveUpdate::on_resume(100, the_timing);
  LiveUpdate::on_resume(665, saved_message);
  LiveUpdate::on_resume(666, restore_term);
  // begin restoring saved data
  if (LiveUpdate::resume(on_missing) == false) {
    printf("* Not restoring data, because no update has happened\n");
    // .. logic for when there is nothing to resume yet
  }
  
  // listen for telnet clients
  setup_terminal(inet);
  
  // listen for live updates
  auto& server = inet.tcp().bind(666);
  server.on_connect(
  [] (auto conn)
  {
    char* update_blob = new char[1024*1024*10];
    int*  update_size = new int(0);

    // retrieve binary
    conn->on_read(9000,
    [conn, update_blob, update_size] (net::tcp::buffer_t buf, size_t n)
    {
      memcpy(update_blob + *update_size, buf.get(), n);
      *update_size += (int) n;

    }).on_close(
    [update_blob, update_size] {
      // we received a binary:
      printf("* New update size: %u b\n", *update_size);
      // save stuff for later
      void save_stuff(Storage);
      // run live update process
      LiveUpdate::begin({update_blob, *update_size}, save_stuff);
      /// We should never return :-) ///
      assert(0 && "!! Update failed !!");
    });
  });

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
  
  // messages received from terminals
  for (auto& msg : savemsg)
      storage.add_string(665, msg);
  // open terminals
  for (auto conn : saveme)
    if (conn->is_connected())
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
void saved_message(Restore thing)
{
  static int n = 0;
  auto str = thing.as_string();
  
  printf("[%d] %s", ++n, str.c_str());
  // re-save it
  savemsg.push_back(str);
}
void on_missing(Restore thing)
{
  printf("Missing resume function for %u\n", thing.get_id());
}

void the_timing(Restore thing)
{
  auto t1   = thing.as_type<int64_t>();
  auto diff = hw::CPU::rdtsc() - t1;

  using namespace std::chrono;
  double  div  = OS::cpu_freq().count() * 1000000.0;
  int64_t time = diff / div * 1000;

  char buffer[256];
  int len = snprintf(buffer, sizeof(buffer),
             "! Boot time in ticks: %lld (%lld ms)\n", diff, time);

  savemsg.emplace_back(buffer, len);
}
void restore_term(Restore thing)
{
  auto& stack = net::Inet4::stack<0> ();
  // restore connection to terminal
  auto conn = thing.as_tcp_connection(stack.tcp());
  setup_terminal_connection(conn);
  printf("Restored terminal connection to %s\n", conn->remote().to_string().c_str());
  
  // send all the messages so far
  for (auto msg : savemsg) {
    conn->write(msg);
  }
}
