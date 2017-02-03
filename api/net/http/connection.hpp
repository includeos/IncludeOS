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
#ifndef HTTP_CONNECTION_HPP
#define HTTP_CONNECTION_HPP

// http
#include "request.hpp"
#include "response.hpp"

#include <net/tcp/connection.hpp>

namespace http {

  class Connection {
  public:
    using TCP_conn      = net::tcp::Connection_ptr;
    using Peer          = net::tcp::Socket;
    using buffer_t      = net::tcp::buffer_t;

  public:
    inline explicit Connection(TCP_conn tcpconn, bool keep_alive = true);

    template <typename TCP>
    explicit Connection(TCP&, Peer);

    inline explicit Connection() noexcept;

    net::tcp::port_t local_port() const noexcept
    { return (tcpconn_) ? tcpconn_->local_port() : 0; }

    Peer peer() const noexcept
    { return peer_; }

    void timeout()
    { tcpconn_->is_closing() ? tcpconn_->abort() : tcpconn_->close(); }

    auto&& tcp() const
    { return tcpconn_; }

    /**
     * @brief      Shutdown the underlying TCP connection
     */
    inline void shutdown();

    /**
     * @brief      Release the underlying TCP connection,
     *             making this connection useless.
     *
     * @return     The underlying TCP connection
     */
    inline TCP_conn release();

    /**
     * @brief      Wether the underlying TCP connection has been released or not
     *
     * @return     true if the underlying TCP connection is released
     */
    bool released() const
    { return tcpconn_ == nullptr; }

    static Connection& empty() noexcept
    {
      static Connection c;
      return c;
    }

    /* Delete copy constructor */
    Connection(const Connection&)             = delete;

    Connection(Connection&&)                  = default;

    /* Delete copy assignment */
    Connection& operator=(const Connection&)  = delete;

    Connection& operator=(Connection&&)       = default;

    virtual ~Connection() {}

  protected:
    TCP_conn          tcpconn_;
    bool              keep_alive_;
    Peer              peer_;

  }; // < class Connection

  inline Connection::Connection(TCP_conn tcpconn, bool keep_alive)
    : tcpconn_{std::move(tcpconn)},
      keep_alive_{keep_alive},
      peer_{tcpconn_->remote()}
  {
    Ensures(tcpconn_ != nullptr);
    debug("<http::Connection> Created %u -> %s %p\n", local_port(), peer().to_string().c_str(), this);
  }

  template <typename TCP>
  Connection::Connection(TCP& tcp, Peer addr)
    : Connection(tcp.connect(addr))
  {
  }

  inline Connection::Connection() noexcept
    : tcpconn_(nullptr),
      keep_alive_(false),
      peer_{}
  {
  }

  inline void Connection::shutdown()
  {
    if(tcpconn_->is_closing())
      tcpconn_->close();
  }

  inline Connection::TCP_conn Connection::release()
  {
    auto copy = tcpconn_;

    // this is expensive and may be unecessary,
    // but just to be safe for now
    copy->setup_default_callbacks();

    tcpconn_ = nullptr;

    return copy;
  }

} // < namespace http

#endif // < HTTP_CONNECTION_HPP
