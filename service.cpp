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
#include "ircd.hpp"

void Service::start() {
  static std::unique_ptr< net::Inet4<VirtioNet> > inet;

  inet = net::new_ipv4_stack(
      {  10, 0,  0, 42 },  // IP
      { 255,255,255, 0 },  // Netmask
      {  10, 0,  0,  1 }); // Gateway
  
  // IRC default port
  static std::vector<std::string> motd;
  motd.push_back("Welcome to the");
  motd.push_back("IncludeOS IRC server");
  motd.push_back("4Head");
  
  auto ircd =
  new IrcServer(*inet.get(), 6667, "irc.includeos.org", "IncludeNet",
  [] () -> const std::vector<std::string>& {
    return motd;
  });
  
  using namespace std::chrono;
  hw::PIT::instance().on_repeated_timeout(5s, 
  [ircd] {
    printf("Conns received %d  Local clients %d\n",  
        ircd->get_counter(STAT_TOTAL_CONNS), ircd->get_counter(STAT_LOCAL_USERS));
  });

  printf("*** IRC SERVICE STARTED *** \n");
}
