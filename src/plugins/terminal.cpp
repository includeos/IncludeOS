// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015 Oslo and Akershus University College of Applied Sciences
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

#include <rapidjson/document.h>
#include <config>
#include <info>
#include <net/inet4>
#include <kernel/terminal.hpp>
#include <kernel/os.hpp>

static void spawn_terminal()
{
  rapidjson::Document doc;
  doc.Parse(Config::get().data());

  if (doc.IsObject() == false || doc.HasMember("terminal") == false)
      throw std::runtime_error("Missing terminal configuration");

  const auto& obj = doc["terminal"];
  // terminal network interface
  const int TERM_NET  = obj["iface"].GetInt();
  auto& inet = net::Super_stack::get<net::IP4>(TERM_NET);
  // terminal TCP port
  const int TERM_PORT = obj["port"].GetUint();
  inet.tcp().listen(TERM_PORT,
    [] (auto conn) {
      static int counter = 0;
      static std::unordered_map<int, Terminal> terms;
      terms.emplace(std::piecewise_construct,
                    std::forward_as_tuple(counter),
                    std::forward_as_tuple(net::Stream_ptr{new net::tcp::Connection::Stream(conn)}));
      auto idx = counter++;
      conn->on_close(
      [idx] () {
        terms.erase(idx);
      });
    });
  INFO("Terminal", "Listening on port %u", TERM_PORT);
}

__attribute__((constructor))
static void feijfeifjeifjeijfei() {
  OS::register_plugin(spawn_terminal, "Terminal plugin");
}
