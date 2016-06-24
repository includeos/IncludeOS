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

// An IP-stack object
std::unique_ptr<net::Inet4<VirtioNet> > inet;

using namespace std::chrono;
#include <kernel/elf.hpp>
#include <profile>

void Service::start()
{
  begin_stack_sampling(1500);
  
  // print stuff every 5 seconds
  hw::PIT::instance().on_repeated_timeout(1000ms, print_stack_sampling);
  
  // boilerplate
  hw::Nic<VirtioNet>& eth0 = hw::Dev::eth<0,VirtioNet>();
  inet = std::make_unique<net::Inet4<VirtioNet> > (eth0, 1);
  inet->network_config(
    { 10,0,0,42 },      // IP
    { 255,255,255,0 },  // Netmask
    { 10,0,0,1 },       // Gateway
    { 8,8,8,8 } );      // DNS

  // Set up a TCP server on port 80
  auto& server = inet->tcp().bind(80);
  server.onConnect(
  [] (auto conn) {
    conn->close();
  });
}
