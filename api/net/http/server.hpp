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
#include <statman>

namespace http {

  // Used in HTTP server - invoked when a Request is received
  using Request_handler   = delegate<void(Request_ptr, Response_writer_ptr)>;

  class Server {
  public:
    using TCP             = net::TCP;
    using TCP_conn        = net::tcp::Connection_ptr;

    using idle_duration   = std::chrono::seconds;

    static constexpr size_t     DEFAULT_BUFSIZE = 2048;
    static const idle_duration  DEFAULT_IDLE_TIMEOUT; // server.cpp, 60s

  private:
    using Connection_set  = std::vector<std::unique_ptr<Server_connection>>;
    using Index_set       = std::vector<size_t>;

  public:
    /**
     * @brief      Creates a HTTP Server on a given TCP instance
     *
     * @param      tcp      The tcp instance (interface)
     * @param[in]  handler  The handler to be invoked when a request is received (optional in constructor)
     * @param[in]  timeout  The duration for how long a connection can be idle (0 = no timeout)
     */
    explicit Server(TCP& tcp, Request_handler handler = nullptr, idle_duration timeout = DEFAULT_IDLE_TIMEOUT);

    /**
     * @brief      Start listening to a port. Expects a Request_handler to be set (on_request)
     *
     * @param[in]  port  The port to listen on
     */
    virtual void listen(uint16_t port);

    /**
     * @brief      Setup handler for when a Request is received
     *
     * @param[in]  handler    A Request_handler
     */
    void on_request(Request_handler handler)
    { on_request_ = std::move(handler); }

    /**
     * @brief      Returns number of connected clients
     *
     * @return     Number of connected clients
     */
    size_t connected_clients() const noexcept
    { return connections_.size() - free_idx_.size(); }

    /**
     * @brief      Creates a response with some predefined values
     *
     * @param[in]  code  The HTTP status code
     *
     * @return     A Response_ptr with predefined values
     */
    Response_ptr create_response(status_t code = http::OK) const;

    virtual ~Server();

  protected:
    delegate<void(TCP_conn)> on_connect;
    void connect(Connection::Stream_ptr stream);

  private:
    friend class Server_connection;

    TCP&            tcp_;
    Request_handler on_request_;
    Connection_set  connections_;
    Index_set       free_idx_;
    bool            keep_alive_;
    Timers::id_t    timer_id_;

    const idle_duration idle_timeout_;

    Stat& stat_conns_;
    Stat& stat_req_rx_;
    Stat& stat_req_bad_;
    Stat& stat_timeouts_;

    void connected(TCP_conn conn) {
      connect(std::make_unique<Connection::Stream>(conn));
    }

    void close(Server_connection&);

    void timeout_clients(int32_t);

    void receive(Request_ptr, status_t code, Server_connection&);

  }; // < class Server

} // < namespace http

#endif // < HTTP_SERVER_HPP
