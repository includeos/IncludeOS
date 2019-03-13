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
#include <net/interfaces>
#include <kernel/terminal.hpp>
#include <os.hpp>
static int counter = 0;
static std::unordered_map<int, Terminal> terms;

static auto& create_connection_from(net::tcp::Connection_ptr conn)
{
  terms.emplace(std::piecewise_construct,
                std::forward_as_tuple(counter),
                std::forward_as_tuple(conn));
  auto idx = counter++;
  conn->on_close(
  [idx] () {
    terms.erase(idx);
  });
  return terms.at(idx);
}

#ifdef USE_LIVEUPDATE
#include "liveupdate.hpp"

void store_terminal(liu::Storage& store, const liu::buffer_t*)
{
  for (auto& kv : terms)
  {
    auto conn = kv.second.get_stream();
    if (conn->is_connected()) {
      store.add_connection(1, conn);
    }
  }
}
void restore_terminal(const int TERM_NET, liu::Restore& restore)
{
  auto& inet = net::Interfaces::get(TERM_NET);
  while (!restore.is_end())
  {
    auto conn = restore.as_tcp_connection(inet.tcp());
    auto& term = create_connection_from(conn);
    term.write("...\nThe system was just updated. Welcome back!\n");
    term.prompt();
    restore.go_next();
  }
}
#endif

static void spawn_terminal()
{
  rapidjson::Document doc;
  doc.Parse(Config::get().data());

  if (doc.IsObject() == false || doc.HasMember("terminal") == false)
      throw std::runtime_error("Missing terminal configuration");

  const auto& obj = doc["terminal"];
  // terminal network interface
  const int TERM_NET  = obj["iface"].GetInt();
  auto& inet = net::Interfaces::get(TERM_NET);
  // terminal TCP port
  const int TERM_PORT = obj["port"].GetUint();
  inet.tcp().listen(TERM_PORT,
    [] (auto conn) {
      create_connection_from(conn);
    });
#ifdef USE_LIVEUPDATE
  if(liu::LiveUpdate::is_resumable())
  {
    liu::LiveUpdate::resume("terminal",
      [TERM_NET] (liu::Restore& restore) {
        restore_terminal(TERM_NET, restore);
      });
    INFO("Terminal", "Restored state");
  }
  liu::LiveUpdate::register_partition("terminal", {&store_terminal});
#endif
  INFO("Terminal", "Listening on port %u", TERM_PORT);
}

__attribute__((constructor))
static void feijfeifjeifjeijfei() {
  os::register_plugin(spawn_terminal, "Terminal plugin");
}
