// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2016 Oslo and Akershus University College of Applied Sciences
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

#include <hw/devices.hpp>
#include <net/inet4.hpp>

// Specialization for IP4
template <>
net::Inet<net::IP4>& net::Super_stack::get<net::IP4>(int N) {
  return *(inet().ip4_stacks_[N]);
}


net::Super_stack::Super_stack()
{
  INFO("Super stack", "Constructing");
  for(auto& nic : hw::Devices::devices<hw::Nic>())
  {
    INFO("Super stack", "Creating stack for Nic %s", nic->ifname().c_str());
    switch(nic->proto()) {

    case(hw::Nic::Proto::ETH) :
      ip4_stacks_.emplace_back(std::unique_ptr<net::Inet4>(new Inet4(*nic)));
      // ip6_stacks come here I guess
    default:
      continue;

    }
  }
}
