// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2016-2017 Oslo and Akershus University College of Applied Sciences
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

#include <net/http/server.hpp>

namespace http {

  const Server::idle_duration Server::DEFAULT_IDLE_TIMEOUT{std::chrono::seconds(60)};

  Server::Server(TCP& tcp, Request_handler cb, idle_duration timeout)
    : tcp_(tcp),
      on_request_(std::move(cb)),
      keep_alive_(true),
      timer_id_(Timers::UNUSED_ID),
      idle_timeout_(timeout)
  {}

  void Server::listen(uint16_t port)
  {
    Expects(on_request_ != nullptr);

    tcp_.bind(port).on_connect({this, &Server::connect});
    INFO("HTTP Server", "Listening on port %u", port);

    using namespace std::chrono;
    timer_id_ = Timers::periodic(30s, 1min, {this, &Server::timeout_clients});
  }

  Response_ptr Server::create_response(status_t code) const
  {
    auto res = make_response();
    res->set_status_code(code);
    return res;
  }

  Server::~Server()
  {
    if(timer_id_ != Timers::UNUSED_ID)
    {
      Timers::stop(timer_id_);
    }
  }

  void Server::connect(TCP_conn conn)
  {
    debug("Connection attempt from %s\n", conn->remote().to_string().c_str());
    // if there is a free spot in connections
    if(free_idx_.size() > 0) {
      auto idx = free_idx_.back();
      Ensures(connections_[idx] == nullptr);
      connections_[idx] = std::make_unique<Server_connection>(*this, conn, idx);
      free_idx_.pop_back();
    }
    // if not, add a new shared ptr
    else {
      connections_.emplace_back(std::make_unique<Server_connection>(*this, conn, connections_.size()));
    }
  }

  void Server::close(Server_connection& conn)
  {
    const auto idx = conn.idx();
    connections_[idx] = nullptr;
    free_idx_.push_back(idx);
  }

  void Server::timeout_clients(int32_t)
  {
    const auto count = idle_timeout_.count();
    for(auto& conn : connections_) {
      if(conn != nullptr and RTC::now() > (conn->idle_since() + count))
        conn->timeout();
    }
  }

  void Server::receive(Request_ptr req, status_t code, Server_connection& conn)
  {
    if(code == OK)
    {
      on_request_(std::move(req), {create_response(code), conn.tcp()});
    }
    else
    {
      // an error occured when parsing
      // call user on_error or something
      conn.send(create_response(code));
    }
  }

}
