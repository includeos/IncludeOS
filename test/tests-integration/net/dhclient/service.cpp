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

//#define DEBUG // Debug supression

#include <os>
#include <list>
#include <net/interfaces>

using namespace net;

void Service::start(const std::string&)
{
  static auto& inet = Interfaces::get(0);
  inet.negotiate_dhcp(10.0, [](bool timeout) {
      if (timeout)
        os::panic("DHCP timed out");

      INFO("DHCP test", "Got IP from DHCP");
      printf("%s\n", inet.ip_addr().str().c_str());
    });
  INFO("DHCP test", "Waiting for DHCP response\n");
}
