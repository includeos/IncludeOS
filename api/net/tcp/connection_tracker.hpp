// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2017 Oslo and Akershus University College of Applied Sciences
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
#ifndef NET_TCP_CONNECTION_TRACKER_HPP
#define NET_TCP_CONNECTION_TRACKER_HPP

#include "packet.hpp"

namespace net {
namespace tcp {

/**
 * @brief      Class for tracking TCP connections by observing a packet flow.
 */
class Connection_tracker {
public:
  enum class State : uint8_t {
    SYN_SENT,
    SYN_RECV,
    ESTABLISHED,
    CLOSE,
    INVALID
  };

  static std::string to_string(State state)
  {
    switch(state)
    {
      case State::SYN_SENT: return "SYN_SENT";
      case State::SYN_RECV: return "SYN_RECV";
      case State::ESTABLISHED: return "ESTABLISHED";
      case State::CLOSE: return "CLOSE";
      default: return "INVALID";
    }
  }

  using Tuple = std::pair<Socket, Socket>;

  struct Entry {
    Socket src;
    Socket dst;
    State  state;

    Entry(const Tuple& t, State s)
      : src{t.first}, dst{t.second}, state{s}
    {}

    std::string to_string() const
    { return src.to_string() + " " + dst.to_string() + " " + Connection_tracker::to_string(state); }

  };

  using Close_handler = delegate<void(const Tuple&)>;
  using Connections = std::map<Tuple, Entry>;

public:
  Close_handler on_close;

  void incoming(const tcp::Packet& pkt)
  {
    //printf("Incoming: %s\n", pkt.to_string().c_str());
    handle({pkt.destination(), pkt.source()}, pkt);
  }

  void outgoing(const tcp::Packet& pkt)
  {
    //printf("Outgoing: %s\n", pkt.to_string().c_str());
    handle({pkt.source(), pkt.destination()}, pkt);
  }

  void handle(const Tuple& tuple, const tcp::Packet& pkt)
  {
    auto it = entries.find(tuple);

    if(it != entries.end())
    {
      do_state(it->second, pkt);
    }
    else
    {
      new_entry(tuple, pkt);
    }
  }

  void new_entry(const Tuple& tuple, const tcp::Packet& pkt)
  {
    if(pkt.isset(SYN) and not pkt.isset(ACK))
    {
      entries.emplace(std::piecewise_construct,
        std::forward_as_tuple(tuple),
        std::forward_as_tuple(tuple, State::SYN_SENT));

      //printf("SYN_SENT\n");
    }
    else
    {
      entries.emplace(std::piecewise_construct,
        std::forward_as_tuple(tuple),
        std::forward_as_tuple(tuple, State::INVALID));
    }
  }

  void do_state(Entry& entry, const tcp::Packet& pkt)
  {
    if(UNLIKELY(pkt.isset(RST)))
    {
      entry.state = State::CLOSE;
      return;
    }

    switch(entry.state)
    {
      case State::SYN_SENT:
      {
        if(pkt.isset(SYN) and pkt.isset(ACK)) {
          entry.state = State::SYN_RECV;
          //printf("Entry: %s\n", entry.to_string().c_str());
        }

        return;
      }

      case State::SYN_RECV:
      {
        if(pkt.isset(ACK)) {
          entry.state = State::ESTABLISHED;
          //printf("Entry: %s\n", entry.to_string().c_str());
        }
      }

      default:
        return;
    }
  }

private:
  Connections entries;

};

} // < namespace tcp
} // < namespace net

#endif

