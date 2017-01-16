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
#ifndef HTTP_CONNECTION_HPP
#define HTTP_CONNECTION_HPP

#include "common.hpp"
#include "request.hpp"
#include "response.hpp"
#include "error.hpp"
#include <net/tcp/connection.hpp>
#include <map>
#include <vector>
#include <delegate>
#include <util/timer.hpp>

namespace http {

  class Connection {
  public:
    using TCP_conn_ptr      = net::tcp::Connection_ptr;
    using Peer              = net::tcp::Socket;
    using buffer_t          = net::tcp::buffer_t;
    using Close_handler     = delegate<void(Connection&)>;
    using timeout_duration  = std::chrono::milliseconds;

  public:

    explicit Connection(TCP_conn_ptr, Close_handler);

    template <typename TCP>
    explicit Connection(TCP&, Peer, Close_handler);

    bool available() const
    { return on_response_ == nullptr && keep_alive_; }

    bool occupied() const
    { return !available(); }

    void send(Request_ptr, Response_handler, const size_t bufsize, timeout_duration = timeout_duration::zero());

    net::tcp::port_t local_port() const
    { return (tcpconn_) ? tcpconn_->local_port() : 0; }

    Peer peer() const
    { return (tcpconn_) ? tcpconn_->remote() : Peer(); }
    //bool operator==(const Connection& other)
    //{ return this == &other; }
    //{ return tcpconn_->local_port() == other.tcpconn_->local_port(); }

  private:
    TCP_conn_ptr      tcpconn_;
    Request_ptr       req_;
    Response_ptr      res_;
    Close_handler     on_close_;
    Response_handler  on_response_;
    Timer             timer_;

    bool keep_alive_;

    void send_request(const size_t bufsize);

    void recv_response(buffer_t buf, size_t len);

    void end_response(Error err = Error::NONE);

    void timeout_request()
    { end_response(Error::TIMEOUT); }

    void close();

  }; // < class Connection

} // < namespace http

#endif // < HTTP_CONNECTION_HPP
