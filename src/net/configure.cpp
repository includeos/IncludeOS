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

//#define NETCONF_DEBUG 1
#ifdef NETCONF_DEBUG
#define PRINT(fmt, ...) printf(fmt, ##__VA_ARGS__)
#else
#define PRINT(fmt, ...) /* fmt */
#endif

namespace net {

struct ipv4_res
{
  ip4::Addr addr;
  ip4::Addr netmask;
};

struct ipv6_res
{
  ip6::Addr addr;
  uint8_t prefix;
};

struct v4_config
{
  ip4::Addr addr;
  ip4::Addr netmask;
  ip4::Addr gateway;
  ip4::Addr dns;
};

struct v6_config
{
  std::vector<ipv6_res>  addr;
  std::vector<ip6::Addr> gateway;
  std::vector<ip6::Addr> dns;

  void clear()
  {
    addr.clear();
    gateway.clear();
    dns.clear();
  }
};

inline bool is_v6(const std::string& str)
{
  Expects(not str.empty() && "Address string can't be empty");
  return str.find(':') != std::string::npos;
}

inline ipv4_res parse_addr4(const std::string& str)
{
  if(auto n = str.rfind('/'); n != std::string::npos)
  {
    uint8_t bits = std::stoi(str.substr(n+1));
    ip4::Addr netmask{ntohl(0xFFFFFFFF << (32ul-bits))};
    //PRINT("Bits %u Netmask %s\n", bits, netmask.to_string().c_str());
    return {str.substr(0, n), netmask};
  }
  else
  {
    return {str, 0};
  }
}

inline ipv6_res parse_addr6(const std::string& str)
{
  if(auto n = str.rfind('/'); n != std::string::npos)
  {
    uint8_t prefix = std::stoi(str.substr(n+1));
    return {str.substr(0, n), prefix};
  }
  else
  {
    return {str, 0};
  }
}

inline void parse_addr(v4_config& cfg4, v6_config& cfg6, std::string str)
{
  if(is_v6(str))
  {
    cfg6.addr.push_back(parse_addr6(str));
    PRINT("v6 Addr: %s Prefix: %u\n",
      cfg6.addr.back().addr.to_string().c_str(), cfg6.addr.back().prefix);
  }
  else
  {
    if(cfg4.addr != 0)
    {
      MYINFO("WARN: Multiple v4 addresses not supported: skipping %s", str.c_str());
      return;
    }

    auto [addr, netmask] = parse_addr4(str);
    cfg4.addr = addr;
    cfg4.netmask = netmask;
    PRINT("v4 Addr: %s Netmask: %s\n",
      cfg4.addr.to_string().c_str(), cfg4.netmask.to_string().c_str());
  }
}

inline void parse_gateway(v4_config& cfg4, v6_config& cfg6, std::string str)
{
  if(is_v6(str))
  {
    cfg6.gateway.push_back(parse_addr6(str).addr);
    PRINT("v6 Gateway: %s\n",
      cfg6.gateway.back().to_string().c_str());
  }
  else
  {
    if(cfg4.gateway != 0)
    {
      MYINFO("WARN: Multiple v4 gateways not supported: skipping %s", str.c_str());
      return;
    }

    cfg4.gateway = parse_addr4(str).addr;
    PRINT("v4 Gateway: %s\n",
      cfg4.gateway.to_string().c_str());
  }
}

inline void parse_dns(v4_config& cfg4, v6_config& cfg6, std::string str)
{
  if(is_v6(str))
  {
    cfg6.dns.push_back(parse_addr6(str).addr);
    PRINT("v6 DNS: %s\n",
      cfg6.dns.back().to_string().c_str());
  }
  else
  {
    if(cfg4.dns != 0)
    {
      MYINFO("WARN: Multiple v4 DNS not supported: skipping %s", str.c_str());
      return;
    }

    cfg4.dns = parse_addr4(str).addr;
    PRINT("v4 DNS: %s\n",
      cfg4.dns.to_string().c_str());
  }
}

inline void config4(Inet& stack, const v4_config& cfg)
{
  Expects(cfg.addr != 0 && "Missing address (v4)");
  Expects(cfg.netmask != 0 && "Missing netmask");
  stack.network_config(cfg.addr, cfg.netmask, cfg.gateway, cfg.dns);
}

inline void config6(Inet& stack, const v6_config& cfg)
{
  // add address
  for(const auto& [addr, prefix] : cfg.addr)
    (prefix) ? stack.add_addr(addr, prefix) : stack.add_addr(addr);

  // add routers
  for(const auto& addr : cfg.gateway)
    stack.ndp().add_router(addr, 0xFFFF); // waiting for API to change

  // add dns
  for(const auto& addr : cfg.dns) {
    stack.set_dns_server6(addr);
    break; // currently only support one
  }
}

template <typename Val, typename Func>
inline void parse(const Val& val, v4_config& cfg4, v6_config& cfg6, Func func)
{
  if(val.IsArray())
  {
    PRINT("Member is array\n");
    for(auto& addrstr : val.GetArray()) {
      func(cfg4, cfg6, addrstr.GetString());
    }
  }
  else {
    func(cfg4, cfg6, val.GetString());
  }
}

void configure(const rapidjson::Value& net)
{
  MYINFO("Configuring network");

  Expects(net.IsArray() && "Member net is not an array");

  auto configs = net.GetArray();
  if(configs.Size() > Interfaces::get().size())
    MYINFO("WARN: Found more configs than there are interfaces");
  // Iterate all interfaces in config
  for(auto& val : configs)
  {
    Expects(val.HasMember("iface")
      && "Missing member iface - don't know which interface to configure");

    auto N = val["iface"].GetInt();

    auto& stack = Interfaces::get(N);

    // if config is not set, just ignore
    if(not val.HasMember("config")) {
      MYINFO("WARN: Config method not set, ignoring");
      continue;
    }

    v4_config v4cfg;
    v6_config v6cfg;

    // "address"
    if(val.HasMember("address"))
    {
      parse(val["address"], v4cfg, v6cfg, &parse_addr);
    }

    // "netmask" (ipv4 only)
    if(v4cfg.netmask == 0 and val.HasMember("netmask"))
      v4cfg.netmask = {val["netmask"].GetString()};

    // "gateway"
    if(val.HasMember("gateway"))
    {
      parse(val["gateway"], v4cfg, v6cfg, &parse_gateway);
    }

    // "dns"
    if(val.HasMember("dns"))
    {
      parse(val["dns"], v4cfg, v6cfg, &parse_dns);
    }

    std::vector<std::string> methods;
    const auto& config = val["config"];

    // parse all the config methods
    if(config.IsArray())
    {
      for(const auto& method : val["config"].GetArray())
        methods.push_back(method.GetString());
    }
    else
    {
      methods.push_back(val["config"].GetString());
    }

    for(const auto& method : methods)
    {
      const double timeout = (val.HasMember("timeout")) ? val["timeout"].GetDouble() : 10.0;

      if(method == "dhcp") {
        stack.negotiate_dhcp(timeout);
        // do DHCPv6
      }
      else if(method == "dhcp4") {
        stack.negotiate_dhcp(timeout);
      }
      else if(method == "dhcp6") {
        MYINFO("WARN: DHCPv6 not supported");
      }
      else if(method == "static") {
        config4(stack, v4cfg);
        config6(stack, v6cfg);
        v6cfg.clear();
      }
      else if(method == "static4") {
        config4(stack, v4cfg);
      }
      else if(method == "static6") {
        config6(stack, v6cfg);
        v6cfg.clear();
      }
      else if(method == "slaac") {
        stack.autoconf_v6();
      }
      else if(method == "dhcp-with-fallback") // TBD...
      {
        auto static_cfg = [v4cfg, &stack] (bool timedout)
        {
          if(timedout) {
            MYINFO("DHCP timeout (%s) - falling back to static configuration", stack.ifname().c_str());
            config4(stack, v4cfg);
          }
        };
        stack.negotiate_dhcp(timeout, static_cfg);
      }
      else {
        MYINFO("WARN: Unrecognized config method \"%s\"", method.c_str());
      }
    }
  }

  MYINFO("Configuration complete");
}

}
