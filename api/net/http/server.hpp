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

  /**
   * @brief      A simple HTTP server.
   */
  class Server {
  public:
    using Request_handler = http::Request_handler;
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
    void listen(const uint16_t port);

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

    /**
     * Return CPU server is hosted on
    **/
    int get_cpuid() const noexcept {
      return tcp_.get_cpuid();
    }

    virtual ~Server();

  protected:
    TCP&            tcp_;

    /**
     * @brief      Binds to a TCP port and sets up a connect event.
     *             This is called from listen()
     *
     * @param[in]  port  The port
     */
    virtual void bind(const uint16_t port);

    /**
     * @brief      Handle a newly connected TCP client.
     *
     * @param[in]  conn  The TCP connection
     */
    virtual void on_connect(TCP_conn conn)
    { connect(std::make_unique<net::tcp::Stream>(std::move(conn))); }

    /**
     * @brief      Connect the stream to the server.
     *
     * @param[in]  stream  The stream
     */
    void connect(Connection::Stream_ptr stream);

  private:
    friend class Server_connection;

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

    /**
     * @brief      Close the given Server_connection
     *
     * @param      <unnamed>  The server connection to be closed
     */
    void close(Server_connection&);

    /**
     * @brief      Timeout (close) all clients that been idle for more than limit.
     *
     * @param[in]  <unnamed>  A timer id (unused)
     */
    void timeout_clients(int32_t);

    /**
     * @brief      Receive a incoming HTTP request
     *
     * @param[in]  <unnamed>  The HTTP reuqest
     * @param[in]  code       The HTTP status code
     * @param      <unnamed>  The server connection which the req arrived from
     */
    void receive(Request_ptr, status_t code, Server_connection&);

  }; // < class Server

  /**
   * Helper function to create an HTTP server on a given TCP instance
   *
   * @param tcp
   *   The tcp instance (interface)
   *
   * @param handler
   *   The handler to be invoked when a request is received (optional)
   *
   * @param timeout
   *   The duration for how long a connection can be idle (0 = no timeout)
   *
   * Usage example:
   * @code
   *   auto& inet = net::Inet::stack<0>();
   *   Expects(inet.is_configured());
   *   auto server = http::make_server(inet.tcp());
   *   server->on_request([](auto req, auto rw){...});
   *   server->listen(80);
   * @endcode
   */
  template<typename = void>
  inline Server_ptr make_server(Server::TCP& tcp, Server::Request_handler handler = nullptr, Server::idle_duration timeout = Server::DEFAULT_IDLE_TIMEOUT) {
    return std::make_unique<Server>(tcp, handler, timeout);
  }

} // < namespace http

#endif // < HTTP_SERVER_HPP
