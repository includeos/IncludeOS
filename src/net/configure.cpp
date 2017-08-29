// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2017 Oslo and Akershus University College of Applied Sciences
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

#include <net/configure.hpp>

#include <net/super_stack.hpp>
#include <info>

#define MYINFO(X,...) INFO("Netconf",X,##__VA_ARGS__)

namespace net {

using Addresses = std::vector<ip4::Addr>;

template <typename T>
Addresses parse_iface(T& obj)
{
  Expects(obj.IsArray());

  // Iface can either be empty: [] or contain 3 to 4 IP's (see Inet static config)
  if(not obj.Empty())
  {
    Expects(obj.Size() >= 3 and obj.Size() <= 4);

    Addresses addresses;

    for(auto& addr : obj.GetArray())
    {
      addresses.emplace_back(std::string{addr.GetString()});
    }

    return addresses;
  }

  return {};
}

inline void config_stack(Inet<IP4>& stack, const Addresses& addrs)
{
  if(addrs.empty())
    return;

  Expects((addrs.size() > 2 and addrs.size() < 5)
    && "A network config needs to be between 3 and 4 addresses");

  stack.network_config(
    addrs[0], addrs[1], addrs[2],
    ((addrs.size() == 4) ? addrs[3] : 0)
    );
}

void configure(const rapidjson::Value& net)
{
  MYINFO("Configuring network");

  Expects(net.IsArray() && "Member net is not an array");

  int N = 0;
  for(auto& val : net.GetArray())
  {
    if(N >= static_cast<int>(Super_stack::inet().ip4_stacks().size()))
      break;

    auto& stack = Super_stack::get<IP4>(N);
    // static configs
    if(val.IsArray())
    {
      config_stack(stack, parse_iface(val));
    }
    // only negotiate if "dhcp"
    else if(val.IsString() and strcmp(val.GetString(), "dhcp") == 0)
    {
      stack.negotiate_dhcp(5.0);
    }
    // else leave it uninitialized
    ++N;
  }

  MYINFO("Configuration complete");
}

}
