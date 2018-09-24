// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015-2016 Oslo and Akershus University College of Applied Sciences
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

#pragma once
#ifndef DASHBOARD_COMPONENTS_TCP_HPP
#define DASHBOARD_COMPONENTS_TCP_HPP

#include "../component.hpp"

#include <net/inet>
#include <net/tcp/tcp.hpp>

namespace mana {
namespace dashboard {

class TCP : public Component {

public:

  TCP(net::TCP& tcp)
   : tcp_{tcp}
  {}

  std::string key() const override
  { return "tcp"; }

  void serialize(Writer& writer) override {
    writer.StartObject();

    writer.Key("address");
    writer.String(tcp_.address().to_string());

    writer.Key("ifname");
    writer.String(tcp_.stack().ifname());

    // Listeners
    writer.Key("listeners");
    writer.StartArray();
    auto& listeners = tcp_.listeners();
    for(auto it = listeners.begin(); it != listeners.end(); ++it)
    {
      auto& listener = *(it->second);
      serialize_listener(writer, listener);
    }
    writer.EndArray();

    // Connections
    writer.Key("connections");
    writer.StartArray();
    for(auto it : tcp_.connections())
    {
      auto& conn = *(it.second);
      serialize_connection(writer, conn);
    }
    writer.EndArray();

    writer.EndObject();
  }

  static void serialize_connection(Writer& writer, const net::tcp::Connection& conn) {
    writer.StartObject();

    writer.Key("local");
    writer.String(conn.local().to_string());

    writer.Key("remote");
    writer.String(conn.remote().to_string());

    writer.Key("bytes_rx");
    //writer.Uint64(conn.bytes_received());
    writer.Uint(0);

    writer.Key("bytes_tx");
    //writer.Uint64(conn.bytes_transmitted());
    writer.Uint(0);

    writer.Key("state");
    writer.String(conn.state().to_string());

    writer.EndObject();
  }

  static void serialize_listener(Writer& writer, const net::tcp::Listener& listener) {
    writer.StartObject();

    writer.Key("port");
    writer.Uint(listener.port());

    writer.Key("syn_queue");
    writer.StartArray();
    for(auto conn_ptr : listener.syn_queue())
    {
      serialize_connection(writer, *conn_ptr);
    }
    writer.EndArray();

    writer.EndObject();
  }

private:
  net::TCP& tcp_;

};

} // < namespace dashboard
} // < namespace mana

#endif
