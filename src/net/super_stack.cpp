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

namespace net
{

// Specialization for IP4
template <>
Inet<IP4>& Super_stack::create<IP4>(hw::Nic& nic, int N, int sub)
{
  INFO("Network", "Creating stack for %s on %s",
        nic.driver_name(), nic.device_name().c_str());

  auto& stacks = inet().ip4_stacks_.at(N);

  auto it = stacks.find(sub);
  if(it != stacks.end() and it->second != nullptr) {
    throw Super_stack_err{"Stack already exists ["
      + std::to_string(N) + "," + std::to_string(sub) + "]"};
  }

  auto inet = [&nic]()->auto {
    switch(nic.proto()) {
    case hw::Nic::Proto::ETH:
      return std::make_unique<Inet4>(nic);
    default:
      throw Super_stack_err{"Nic not supported"};
    }
  }();

  Ensures(inet != nullptr);

  stacks[sub] = std::move(inet);

  return *stacks[sub];
}

// Specialization for IP4
template <>
Inet<IP4>& Super_stack::get<IP4>(int N)
{
  if (N < 0 || N >= (int) hw::Devices::devices<hw::Nic>().size())
    throw Stack_not_found{"No IP4 stack found for [" + std::to_string(N) + "] (missing driver?)"};

  auto& stacks = inet().ip4_stacks_.at(N);

  if(stacks[0] != nullptr)
    return *stacks[0];

  // create network stack
  auto& nic = hw::Devices::get<hw::Nic>(N);
  return inet().create<IP4>(nic, N, 0);
}

// Specialization for IP4
template <>
Inet<IP4>& Super_stack::get<IP4>(int N, int sub)
{
  if (N < 0 || N >= (int) hw::Devices::devices<hw::Nic>().size())
    throw Stack_not_found{"No IP4 stack found for [" + std::to_string(N) + "] (missing driver?)"};

  auto& stacks = inet().ip4_stacks_.at(N);

  auto it = stacks.find(sub);

  if(it != stacks.end()) {
    Expects(it->second != nullptr && "Creating empty subinterfaces doesn't make sense");
    return *it->second;
  }

  throw Stack_not_found{"Stack not found ["
      + std::to_string(N) + "," + std::to_string(sub) + "]"};
}

Super_stack::Super_stack()
{
  if (hw::Devices::devices<hw::Nic>().empty())
    INFO("Network", "No registered network interfaces found");

  for (size_t i = 0; i < hw::Devices::devices<hw::Nic>().size(); i++) {
    ip4_stacks_.emplace_back(Stacks<IP4>{});
    ip4_stacks_.back()[0] = nullptr;
  }
}

}
