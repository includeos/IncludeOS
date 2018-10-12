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

#include <net/interfaces.hpp>
#include <info>

#define MYINFO(X,...) INFO("Netconf",X,##__VA_ARGS__)

namespace net {

using Addresses = std::vector<ip4::Addr>;

template <typename T>
Addresses parse_iface(T& obj)
{
  if(not obj.HasMember("address") and not obj.HasMember("netmask"))
    return {};
  Expects(obj.HasMember("address"));
  Expects(obj.HasMember("netmask"));

  ip4::Addr address{obj["address"].GetString()};
  ip4::Addr netmask{obj["netmask"].GetString()};
  ip4::Addr gateway = (obj.HasMember("gateway")) ? ip4::Addr{obj["gateway"].GetString()} : 0;
  ip4::Addr dns = (not obj.HasMember("dns")) ? gateway : ip4::Addr{obj["dns"].GetString()};

  Addresses addresses{address, netmask, gateway, dns};
  return addresses;
}

inline void config_stack(Inet& stack, const Addresses& addrs)
{
  if(addrs.empty()) {
    MYINFO("! WARNING: No config for stack %s", stack.ifname().c_str());
    return;
  }

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

  auto configs = net.GetArray();
  if(configs.Size() > Interfaces::instance().stacks().size())
    MYINFO("! WARNING: Found more configs than there are interfaces");
  // Iterate all interfaces in config
  for(auto& val : configs)
  {
    Expects(val.HasMember("iface")
      && "Missing member iface - don't know which interface to configure");

    auto N = val["iface"].GetInt();

    auto& stack = Interfaces::get(N);

    // if config is not set, just ignore
    if(not val.HasMember("config")) {
      MYINFO("NOTE: Config method not set, ignoring");
      continue;
    }

    std::string method = val["config"].GetString();

    double timeout = (val.HasMember("timeout")) ? val["timeout"].GetDouble() : 10.0;

    if(method == "dhcp")
    {
      stack.negotiate_dhcp(timeout);
    }
    else if(method == "static")
    {
      config_stack(stack, parse_iface(val));
    }
    else if(method == "dhcp-with-fallback")
    {
      auto addresses = parse_iface(val);
      auto static_cfg = [addresses, &stack] (bool timedout)
      {
        if(timedout) {
          MYINFO("DHCP timeout (%s) - falling back to static configuration", stack.ifname().c_str());
          config_stack(stack, addresses);
        }
      };
      stack.negotiate_dhcp(timeout, static_cfg);
    }
  }

  MYINFO("Configuration complete");
}

}
