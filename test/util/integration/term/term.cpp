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
#include <stdio.h>
#include <cassert>

#include <net/inet4>
std::unique_ptr<net::Inet4<VirtioNet> > inet;

#include <term>
std::unique_ptr<Terminal> term;

#include <memdisk>

void Service::start()
{
  // boilerplate
  hw::Nic<VirtioNet>& eth0 = hw::Dev::eth<0,VirtioNet>();
  inet = std::make_unique<net::Inet4<VirtioNet> >(eth0);
  inet->network_config(
     { 10,0,0,42 },      // IP
     { 255,255,255,0 },  // Netmask
     { 10,0,0,1 },       // Gateway
     { 8,8,8,8 } );      // DNS

  INFO("TERM", "Running tests for Terminal");
  auto disk = fs::new_shared_memdisk();
  assert(disk);

  // auto-mount filesystem
  disk->mount(
  [disk] (fs::error_t err)
  {
    CHECKSERT(!err, "Filesystem auto-mounted");
    
    /// terminal ///
    #define SERVICE_TELNET    23
    auto& tcp = inet->tcp();
    auto& server = tcp.bind(SERVICE_TELNET);
    server.onConnect(
    [disk] (auto client) {
      // create terminal with open TCP connection
      term = std::make_unique<Terminal> (client);
      term->add_disk_commands(disk);

      /// work on network commands ///
      // add 'ifconfig' command
      term->add(
        "ifconfig", "Configure a network interface",
      [] (const std::vector<std::string>&) -> int {
        term->write("Link encap:%s\r\n", inet->tcp().status().c_str());
        return 0;
      });
      // add 'netstat' command
      term->add(
        "netstat", "Print network connections",
      [] (const std::vector<std::string>&) -> int {
        term->write("%s\r\n", inet->tcp().status().c_str());
        return 0;
      });
    });

    INFO("TERM", "Connect to terminal with $ telnet %s ",
        inet->ip_addr().str().c_str());
    /// terminal ///
  });
}
