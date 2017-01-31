// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2016 Oslo and Akershus University College of Applied Sciences
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
#ifndef HTTP_SERVER_HPP
#define HTTP_SERVER_HPP

// http
#include "common.hpp"
#include "server_connection.hpp"
#include "response_writer.hpp"

#include <net/tcp/tcp.hpp>
#include <timers>
#include <vector>

namespace http {

  using Request_handler   = delegate<void(Request_ptr, Response_writer)>;

  class Server {
  public:
    using TCP             = net::TCP;
    using TCP_conn        = net::tcp::Connection_ptr;

    using idle_duration = std::chrono::seconds;

    static constexpr size_t     DEFAULT_BUFSIZE = 2048;
    static const idle_duration  DEFAULT_IDLE_TIMEOUT; // server.cpp, 60s

  private:
    using Connection_set  = std::vector<std::unique_ptr<Server_connection>>;
    using Index_set       = std::vector<size_t>;

  public:
    explicit Server(TCP& tcp, Request_handler handler = nullptr, idle_duration timeout = DEFAULT_IDLE_TIMEOUT);

    void listen(uint16_t port);

    void on_request(Request_handler cb)
    { on_request_ = std::move(cb); }

    size_t active_clients() const noexcept
    { return connections_.size() - free_idx_.size(); }

    Response_ptr create_response(status_t code = http::OK) const;

    ~Server();

  private:
    friend class Server_connection;

    TCP&            tcp_;
    Request_handler on_request_;
    Connection_set  connections_;
    Index_set       free_idx_;
    bool            keep_alive_;
    Timers::id_t    timer_id_;

    const idle_duration idle_timeout_;

    void connect(TCP_conn conn);

    void close(Server_connection&);

    void timeout_clients(int32_t);

    void receive(Request_ptr, status_t code, Server_connection&);

  }; // < class Server

} // < namespace http

#endif // < HTTP_SERVER_HPP
