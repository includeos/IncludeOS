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
#include <info>

#define MYINFO(X,...) INFO("Autoconf",X,##__VA_ARGS__)

namespace net {

const std::string autoconf::DEFAULT_CFG{"config.json"};

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

inline std::string failure(const std::string& err)
{
  INFO2("Aborted: %s", err.c_str());
  return {};
}

inline std::string read_json_from_memdisk(const std::string& file)
{
  MYINFO("Loading config \"%s\" from memdisk", file.c_str());

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

void autoconf::load()
{
  load(DEFAULT_CFG);
}

void autoconf::load(const std::string& file)
{
  auto json = read_json_from_memdisk(file);

  if(not json.empty())
  {
    configure(json);
  }
}

void autoconf::configure(const std::string& json)
{
  using namespace rapidjson;
  Document doc;
  doc.Parse(json.data());

  Expects(doc.IsObject() && "Malformed config");

  Expects(doc.HasMember("net") && "Missing member \"net\"");

  configure(doc["net"]);
}

void autoconf::configure(const rapidjson::Value& net)
{
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
    else if(val.IsString() and strcmp(val.GetString(), "dhcp") == 0)
    {
      stack.negotiate_dhcp(5.0);
    }
    ++N;
  }

  MYINFO("Configure complete");
}

}


