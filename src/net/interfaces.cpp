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

#include <net/interfaces.hpp>
#include <hal/machine.hpp>

namespace net
{

Inet& Interfaces::create(hw::Nic& nic, int N, int sub)
{
  INFO("Network", "Creating stack for %s on %s (MTU=%u)",
        nic.driver_name(), nic.device_name().c_str(), nic.MTU());

  auto& stacks = instance().stacks_.at(N);

  auto it = stacks.find(sub);
  if(it != stacks.end() and it->second != nullptr) {
    throw Interfaces_err{"Stack already exists ["
      + std::to_string(N) + "," + std::to_string(sub) + "]"};
  }

  auto inet = [&nic]()->auto {
    switch(nic.proto()) {
    case hw::Nic::Proto::ETH:
      return std::make_unique<Inet>(nic);
    default:
      throw Interfaces_err{"Nic not supported"};
    }
  }();

  Ensures(inet != nullptr);

  stacks[sub] = std::move(inet);

  return *stacks[sub];
}

Inet& Interfaces::get(int N)
{
  if (N < 0 || N >= os::machine().count<hw::Nic>())
    throw Stack_not_found{"No IP4 stack found with index: " + std::to_string(N) +
      ". Missing device (NIC) or driver."};

  auto& stacks = instance().stacks_.at(N);

  if(stacks[0] != nullptr)
    return *stacks[0];

  // create network stack
  auto& nic = os::machine().get<hw::Nic>(N);
  return instance().create(nic, N, 0);
}

Inet& Interfaces::get(int N, int sub)
{
  if (N < 0 || N >= os::machine().count<hw::Nic>())
    throw Stack_not_found{"No IP4 stack found with index: " + std::to_string(N) +
      ". Missing device (NIC) or driver."};

  auto& stacks = instance().stacks_.at(N);

  auto it = stacks.find(sub);

  if(it != stacks.end()) {
    Expects(it->second != nullptr && "Creating empty subinterfaces doesn't make sense");
    return *it->second;
  }

  throw Stack_not_found{"IP4 Stack not found ["
      + std::to_string(N) + "," + std::to_string(sub) + "]"};
}


ssize_t Interfaces::get_nic_index(const MAC::Addr& mac)
{
  ssize_t index = -1;
  auto nics = os::machine().get<hw::Nic>();
  for (size_t i = 0; i < nics.size(); i++) {
    const hw::Nic& nic = nics.at(i);
    if (nic.mac() == mac) {
      index = i;
      break;
    }
  }

  // If no NIC, no point looking more
  if(index < 0)
    throw Interfaces_err{"No NIC found with MAC address " + mac.to_string()};

  return index;
}

Inet& Interfaces::get(const std::string& mac)
{
  auto index = get_nic_index(mac);
  auto& stacks = instance().stacks_.at(index);
  auto& stack = stacks[0];
  if(stack != nullptr) {
    Expects(stack->link_addr() == MAC::Addr(mac.c_str()));
    return *stack;
  }

  // If not found, create
  return instance().create(os::machine().get<hw::Nic>(index), index, 0);
}

// Duplication of code to keep sanity intact
Inet& Interfaces::get(const std::string& mac, int sub)
{
  auto index = get_nic_index(mac);
  return get(index, sub);
}

Interfaces::Interfaces()
{
  if (os::machine().count<hw::Nic>() == 0)
    INFO("Network", "No registered network interfaces found");

  for (size_t i = 0; i < os::machine().get<hw::Nic>().size(); i++) {
    stacks_.emplace_back();
    stacks_.back()[0] = nullptr;
  }
}

}
