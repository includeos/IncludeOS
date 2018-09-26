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
#include <net/inet>
#include <hw/mac_addr.hpp>

namespace net
{

Inet& Super_stack::create(hw::Nic& nic, int N, int sub)
{
  INFO("Network", "Creating stack for %s on %s (MTU=%u)",
        nic.driver_name(), nic.device_name().c_str(), nic.MTU());

  auto& stacks = inet().stacks_.at(N);

  auto it = stacks.find(sub);
  if(it != stacks.end() and it->second != nullptr) {
    throw Super_stack_err{"Stack already exists ["
      + std::to_string(N) + "," + std::to_string(sub) + "]"};
  }

  auto inet = [&nic]()->auto {
    switch(nic.proto()) {
    case hw::Nic::Proto::ETH:
      return std::make_unique<Inet>(nic);
    default:
      throw Super_stack_err{"Nic not supported"};
    }
  }();

  Ensures(inet != nullptr);

  stacks[sub] = std::move(inet);

  return *stacks[sub];
}

Inet& Super_stack::get(int N)
{
  if (N < 0 || N >= (int) hw::Devices::devices<hw::Nic>().size())
    throw Stack_not_found{"No IP4 stack found with index: " + std::to_string(N) +
      ". Missing device (NIC) or driver."};

  auto& stacks = inet().stacks_.at(N);

  if(stacks[0] != nullptr)
    return *stacks[0];

  // create network stack
  auto& nic = hw::Devices::get<hw::Nic>(N);
  return inet().create(nic, N, 0);
}

Inet& Super_stack::get(int N, int sub)
{
  if (N < 0 || N >= (int) hw::Devices::devices<hw::Nic>().size())
    throw Stack_not_found{"No IP4 stack found with index: " + std::to_string(N) +
      ". Missing device (NIC) or driver."};

  auto& stacks = inet().stacks_.at(N);

  auto it = stacks.find(sub);

  if(it != stacks.end()) {
    Expects(it->second != nullptr && "Creating empty subinterfaces doesn't make sense");
    return *it->second;
  }

  throw Stack_not_found{"IP4 Stack not found ["
      + std::to_string(N) + "," + std::to_string(sub) + "]"};
}

Inet& Super_stack::get(const std::string& mac)
{
  MAC::Addr link_addr{mac.c_str()};
  auto index = hw::Devices::nic_index(link_addr);

  // If no NIC, no point looking more
  if(index < 0)
    throw Stack_not_found{"No NIC found with MAC address " + mac};

  auto& stacks = inet().stacks_.at(index);
  auto& stack = stacks[0];
  if(stack != nullptr) {
    Expects(stack->link_addr() == link_addr);
    return *stack;
  }

  // If not found, create
  return inet().create(hw::Devices::nic(index), index, 0);
}

// Duplication of code to keep sanity intact
Inet& Super_stack::get(const std::string& mac, int sub)
{
  auto index = hw::Devices::nic_index(mac.c_str());

  if(index < 0)
    throw Stack_not_found{"No NIC found with MAC address " + mac};

  return get(index, sub);
}

Super_stack::Super_stack()
{
  if (hw::Devices::devices<hw::Nic>().empty())
    INFO("Network", "No registered network interfaces found");

  for (size_t i = 0; i < hw::Devices::devices<hw::Nic>().size(); i++) {
    stacks_.emplace_back();
    stacks_.back()[0] = nullptr;
  }
}

}
