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
#ifndef NET_TCP_LISTENER_HPP
#define NET_TCP_LISTENER_HPP

#include <deque>

#include "common.hpp"
#include "connection.hpp"
#include "packet.hpp"

namespace net {
namespace tcp {

class Listener {
public:
  using AcceptCallback       = delegate<bool(Socket)>;
  using ConnectCallback      = Connection::ConnectCallback;
  using CleanupCallback      = Connection::CleanupCallback;

  friend class net::TCP;

public:

  Listener(TCP& host, port_t port);

  inline Listener& on_accept(AcceptCallback cb)
  {
    on_accept_ = cb;
    return *this;
  }

  inline Listener& on_connect(ConnectCallback cb)
  {
    on_connect_ = cb;
    return *this;
  }

  inline bool syn_queue_full() const
  { return syn_queue_.size() >= SYN_QUEUE_LIMIT; }


private:
  TCP& host_;
  const port_t port_;
  static constexpr size_t SYN_QUEUE_LIMIT = 5;

  std::deque<Connection_ptr> syn_queue_;

  /** */
  AcceptCallback on_accept_;

  /** */
  ConnectCallback on_connect_;

  bool default_on_accept(Socket)
  { return true; }

  void default_on_connect(Connection_ptr) {}

  void segment_arrived(Packet_ptr);

  void remove(Connection_ptr);

  void connected(Connection_ptr);

  std::string to_string() const;

};

} // < namespace tcp
} // < namespace net

#endif // < NET_TCP_LISTENER_HPP
