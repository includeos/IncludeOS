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
#include <net/interfaces>

using namespace net;

void Service::start(const std::string&)
{
  static auto& inet = net::Interfaces::get(0);
  inet.autoconf_v6(1, [](bool completed) {
    if (!completed) {
       os::panic("Auto-configuration of IP address failed");
    }
    INFO("Slaac test", "Got IP from Auto-configuration");
    printf("%s\n", inet.ip6_addr().str().c_str());
  });
  INFO("Slaac test", "Waiting for Auto-configuration");
}
