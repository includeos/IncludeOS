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
#include <net/interfaces>
#include <terminal>

void Service::start()
{
  auto& inet = net::Interfaces::get(0);
  inet.network_config(
     { 10,0,0,63 },      // IP
     { 255,255,255,0 },  // Netmask
     { 10,0,0,1 });      // GW

  // add 'netstat' command
  Terminal::register_program(
     "netstat", {"Print network connections",
  [&inet] (Terminal& term, const std::vector<std::string>&) -> int {
     term.write("%s\r\n", inet.tcp().status().c_str());
     printf("SUCCESS\n");
     return 0;
  }});

  #define SERVICE_TELNET    23
  inet.tcp().listen(SERVICE_TELNET,
  [] (auto client) {
    // create terminal with open TCP connection
    static std::unique_ptr<Terminal> term = nullptr;
    term = std::make_unique<Terminal> (client);
  });
  INFO("TERM", "Connect to terminal with $ telnet %s ",
                inet.ip_addr().str().c_str());
}
