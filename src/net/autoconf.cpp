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

#include <net/autoconf.hpp>

#include <net/super_stack.hpp>
#include <memdisk>
#include <rapidjson/document.h>
#include <info>

#define MYINFO(X,...) INFO("Autoconf",X,##__VA_ARGS__)

#ifndef RAPIDJSON_HAS_STDSTRING
  #define RAPIDJSON_HAS_STDSTRING 1
#endif

#ifndef RAPIDJSON_THROWPARSEEXCEPTION
  #define RAPIDJSON_THROWPARSEEXCEPTION 1
#endif

namespace net {

using Addresses = std::vector<ip4::Addr>;
using Configs = std::vector<Addresses>;

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

inline Configs parse_configs(const std::string& json)
{
  using namespace rapidjson;
  Document doc;
  doc.Parse(json.data());

  Expects(doc.IsArray());

  Configs cfgs;
  for(auto& addrs : doc.GetArray())
  {
    cfgs.push_back(parse_iface(addrs));
  }

  return cfgs;
}

inline std::string failure(const std::string& err)
{
  INFO2("Aborted: %s", err.c_str());
  return {};
}

inline std::string load_config(const std::string& file)
{
  MYINFO("Loading config \"%s\" from disk", file.c_str());

  auto& disk = fs::memdisk();

  disk.init_fs([] (auto, auto&) {});

  if(not disk.fs_ready())
  {
    return failure("No memdisk found");
  }

  auto dirent = disk.fs().stat(file);

  if(not dirent.is_file())
  {
    return failure("File not found");
  }

  if(not dirent.size())
  {
    return failure("File is empty");
  }

  return dirent.read();
}


void autoconf::with_dhcp(const std::string& file)
{
  MYINFO("Configuring interfaces (with DHCP)");
  auto cfg = load_config(file);

  if(not cfg.empty())
  {

  }
  else
  {
    only_dhcp();
  }
}

void autoconf::without_dhcp(const std::string& file)
{
  MYINFO("Configuring interfaces (without DHCP)");
  auto str = load_config(file);

  if(not str.empty())
  {
    auto cfgs = parse_configs(str);
    MYINFO("Found config for %lu stacks", cfgs.size());

    for(auto& cfg : cfgs)
    {

    }

    int no_stacks = static_cast<int>(Super_stack::inet().ip4_stacks().size());
    for(int i = 0; i < no_stacks && i < static_cast<int>(cfgs.size()); ++i)
    {
      config_stack(Super_stack::get<IP4>(i), cfgs[i]);
    }
  }
}

void autoconf::only_dhcp()
{
  MYINFO("Configuring interfaces (DHCP only)");
  for(auto& stack : Super_stack::inet().ip4_stacks())
  {
    stack->negotiate_dhcp(5.0);
  }
}

}


