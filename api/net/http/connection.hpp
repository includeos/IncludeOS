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

    net::tcp::port_t local_port() const noexcept
    { return (tcpconn_) ? tcpconn_->local_port() : 0; }

    Peer peer() const noexcept
    { return (tcpconn_) ? tcpconn_->remote() : Peer(); }

    void timeout()
    { tcpconn_->is_closing() ? tcpconn_->abort() : tcpconn_->close(); }

    auto&& tcp() const
    { return tcpconn_; }

  protected:
    TCP_conn          tcpconn_;
    bool              keep_alive_;

  }; // < class Connection

  inline Connection::Connection(TCP_conn tcpconn, bool keep_alive)
    : tcpconn_{std::move(tcpconn)},
      keep_alive_{keep_alive}
  {
    Ensures(tcpconn_ != nullptr);
    debug("<http::Connection> Created %u -> %s %p\n", local_port(), peer().to_string().c_str(), this);
  }

  template <typename TCP>
  Connection::Connection(TCP& tcp, Peer addr)
    : Connection(tcp.connect(addr))
  {
  }

} // < namespace http

#endif // < HTTP_CONNECTION_HPP
