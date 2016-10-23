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
#include "update.hpp"

net::tcp::Connection_ptr deserialize_connection(void* addr, net::TCP& tcp);

std::vector<net::tcp::Connection_ptr> saveme;

void setup_terminal(net::Inet4& inet)
{
  // mini terminal
  printf("Setting up terminal, since we have not updated yet\n");
  
  auto& term = inet.tcp().bind(6667);
  term.on_connect(
  [] (auto conn) {
    saveme.push_back(conn);
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
  void restore_term(Restore);
  void on_missing(Restore);

  LiveUpdate::on_resume(1,   the_string);
  LiveUpdate::on_resume(2,   the_buffer);
  LiveUpdate::on_resume(666, restore_term);
  LiveUpdate::resume(on_missing);
}

void save_stuff(Storage storage)
{
  char buffer[] = "Just some random buffer";
  /// without string: 0x103890
  /// with string:    0x1038f0
  storage.add_string(1, "Some string :(");
  storage.add_string(1, "Some other string :(");
  storage.add_buffer(2, {buffer, sizeof(buffer)});
  
  for (auto conn : saveme)
      storage.add_connection(666, conn);
}

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
void restore_term(Restore thing)
{
  auto& stack = net::Inet4::stack<0> ();
  auto conn = thing.as_tcp_connection(stack.tcp());
  printf("Restored terminal connection to %s\n", conn->remote().to_string().c_str());
  
  // restore callback
  conn->on_read(1024,
  [conn] (net::tcp::buffer_t buf, size_t n)
  {
    printf("Received RESTORED message: %.*s\n", n, buf.get());
  });
}
