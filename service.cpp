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


#include "elf.h"

void begin_update(const char* blob, size_t size)
{
  // move binary to 16mb
  void* update_area = (void*) 0x1000000;
  memcpy(update_area, blob, size);
  
  // discover entry point
  
}

void Service::start(const std::string&)
{
  static char*  update_blob = new char[1024*1024*10];
  static size_t update_size = 0;

  auto& inet = net::Inet4::ifconfig<0>(
        { 10,0,0,42 },     // IP
        { 255,255,255,0 }, // Netmask
        { 10,0,0,1 },      // Gateway
        { 10,0,0,1 });     // DNS
  
  auto& server = inet.tcp().bind(666);
  server.on_connect(
  [] (auto conn)
  {
    conn->on_read(9000,
    [conn] (net::tcp::buffer_t buf, size_t n)
    {
      memcpy(update_blob + update_size, buf.get(), n);
      update_size += n;
      
    }).on_close(
    [] {
      printf("New update size: %u b\n", update_size);
      begin_update(update_blob, update_size);
    });
  });
}
