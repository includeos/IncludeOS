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

#include <net/tcp/stream.hpp>

namespace http {

  class Connection {
  public:
    using Stream        = net::Stream;
    using Stream_ptr    = std::unique_ptr<Stream>;
    using Peer          = net::Socket;
    using buffer_t      = net::Stream::buffer_t;

  public:
    inline explicit Connection(Stream_ptr stream, bool keep_alive = true);

    template <typename TCP>
    explicit Connection(TCP&, Peer);

    inline explicit Connection() noexcept;

    uint16_t local_port() const noexcept
    { return (stream_) ? stream_->local().port() : 0; }

    Peer peer() const noexcept
    { return peer_; }

    void timeout()
    { stream_->close(); }

    auto& stream() const
    { return stream_; }

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
    inline Stream_ptr release();

    /**
     * @brief      Whether the underlying TCP connection has been released or not
     *
     * @return     true if the underlying TCP connection is released
     */
    bool released() const
    { return stream_ == nullptr; }

    static Connection& empty() noexcept
    {
      static Connection c;
      return c;
    }

    bool keep_alive() const
    { return keep_alive_; }

    void keep_alive(bool keep_alive)
    { keep_alive_ = keep_alive; }

    void end();

    /* Delete copy constructor */
    Connection(const Connection&)             = delete;

    Connection(Connection&&)                  = default;

    /* Delete copy assignment */
    Connection& operator=(const Connection&)  = delete;

    Connection& operator=(Connection&&)       = default;

    virtual ~Connection() {}

  protected:
    Stream_ptr        stream_;
    bool              keep_alive_;
    Peer              peer_;

    virtual void close() {}

  }; // < class Connection

  inline Connection::Connection(Stream_ptr stream, bool keep_alive)
    : stream_{std::move(stream)},
      keep_alive_{keep_alive},
      peer_{stream_->remote()}
  {
    Ensures(stream_ != nullptr);
    debug("<http::Connection> Created %u -> %s %p\n", local_port(), peer().to_string().c_str(), this);
  }

  template <typename TCP>
  Connection::Connection(TCP& tcp, Peer addr)
    : Connection(std::make_unique<net::tcp::Stream>(tcp.connect(addr)))
  {
  }

  inline Connection::Connection() noexcept
    : stream_(nullptr),
      keep_alive_(false),
      peer_{}
  {
  }

  inline void Connection::shutdown()
  {
    if(not released() and not stream_->is_closing())
      stream_->close();
  }

  inline Connection::Stream_ptr Connection::release()
  {
    auto copy = std::move(stream_);

    // reset delegates before handing out stream
    copy->reset_callbacks();

    return copy;
  }

  inline void Connection::end()
  {
    if(released())
      close();
    else if(!keep_alive_)
      shutdown();
  }

} // < namespace http

#endif // < HTTP_CONNECTION_HPP
