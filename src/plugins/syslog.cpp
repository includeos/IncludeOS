// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2018 Oslo and Akershus University College of Applied Sciences
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

// This file contains a plugin for automatically mounting
// a given syslog implementation to the virtual filesystem (VFS)

#include <config>
#include <fs/vfs.hpp>
#include <posix/syslog_print_socket.hpp>
#include <posix/syslog_udp_socket.hpp>
#include <net/interfaces.hpp>

#ifndef RAPIDJSON_HAS_STDSTRING
  #define RAPIDJSON_HAS_STDSTRING 1
#endif

#include <rapidjson/document.h>
#include <rapidjson/writer.h>

static std::unique_ptr<Unix_FD_impl> syslog_impl = nullptr;
const static std::string  default_path{"/dev/log"};
const static uint16_t     default_port{6514};

static void mount_print_sock(const std::string& path)
{
  INFO("Syslog", "Mounting Syslog Print backend on %s", path.c_str());
  syslog_impl.reset(new Syslog_print_socket());
  fs::mount(path, *syslog_impl, "Syslog Print Unix Backend");
}

static void mount_udp_sock(const std::string& path, net::Inet& stack,
                           const net::ip4::Addr addr, const uint16_t port)
{
  INFO("Syslog", "Mounting Syslog UDP backend on %s", path.c_str());
  INFO2("%s:%u @ %s", addr.to_string().c_str(), port, stack.ifname().c_str());
  syslog_impl.reset(new Syslog_UDP_socket(stack, addr, port));
  fs::mount(path, *syslog_impl, "Syslog UDP Unix Backend");
}

static void mount_default_sock()
{
  mount_print_sock(default_path);
}

// Function being run by the OS for mounting resources
static void syslog_mount()
{
  const auto& json = ::Config::get();

  // No config, use default
  if(json.empty())
  {
    mount_default_sock();
    return;
  }

  using namespace rapidjson;
  Document doc;
  doc.Parse(json.data());

  Expects(doc.IsObject() && "Malformed config");

  // No syslog member, use default
  if(not doc.HasMember("syslog"))
  {
    mount_default_sock();
    return;
  }

  const auto& cfg = doc["syslog"];

  // get the path if any, else use default
  const std::string path = (cfg.HasMember("path"))
    ? cfg["path"].GetString() : default_path;

  // if no type are given, use print socket
  if(not cfg.HasMember("type") or not (cfg["type"].GetString() == std::string{"udp"}))
  {
    mount_print_sock(path);
    return;
  }

  // check type (only UDP for now)
  Expects(cfg.HasMember("iface") && "Missing iface (index)");
  Expects(cfg.HasMember("address") && "Missing address");

  auto& stack = net::Interfaces::get(cfg["iface"].GetInt());
  const net::ip4::Addr addr{cfg["address"].GetString()};
  const uint16_t port = cfg.HasMember("port") ?
    cfg["port"].GetUint() : default_port;

  mount_udp_sock(path, stack, addr, port);
}

#include <os>
__attribute__((constructor))
static void syslog_plugin() {
  os::register_plugin(syslog_mount, "Syslog Unix backend");
}
