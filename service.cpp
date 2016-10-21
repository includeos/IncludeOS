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

void save_stuff(Storage storage)
{
  char buffer[] = "Just some random buffer";
  
  storage.add_string(1, "Some string :(");
  storage.add_buffer(2, {buffer, sizeof(buffer)});
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
      LiveUpdate::begin({update_blob, update_size}, save_stuff);
      /// We should never return :-) ///
      assert(0 && "!! Update failed !!");
    });
  });

  /// attempt to resume (if there is anything to resume)
  void the_string(Restore thing);
  void the_buffer(Restore thing);
  void on_missing(Restore);
  
  LiveUpdate::on_resume(1, the_string);
  LiveUpdate::on_resume(2, the_buffer);
  LiveUpdate::resume(on_missing);
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
