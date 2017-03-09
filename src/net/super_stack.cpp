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
net::Inet<net::IP4>& net::Super_stack::get<net::IP4>(int N)
{
  if (N < 0 || N >= (int) hw::Devices::devices<hw::Nic>().size())
    throw Stack_not_found{"No IP4 stack found for [" + std::to_string(N) + "] (missing driver?)"};

  if (inet().ip4_stacks_[N])
      return *inet().ip4_stacks_[N];
  
  // create network stack
  auto& nic = hw::Devices::devices<hw::Nic>()[N];
  
  INFO("Network", "Creating stack for %s on %s", 
        nic->driver_name(), nic->device_name().c_str());
  
  switch(nic->proto()) {
  case hw::Nic::Proto::ETH:
      inet().ip4_stacks_[N].reset(new Inet4(*nic));
      // ip6_stacks come here I guess
      break;
  default:
      break;
  }
  return *inet().ip4_stacks_[N];
}

net::Super_stack::Super_stack()
{
  if (hw::Devices::devices<hw::Nic>().empty())
    INFO("Network", "No registered network interfaces found");

  for (size_t i = 0; i < hw::Devices::devices<hw::Nic>().size(); i++)
    ip4_stacks_.push_back(nullptr);
}
