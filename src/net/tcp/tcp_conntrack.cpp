// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2018 IncludeOS AS, Oslo, Norway
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

//#define CT_DEBUG 1
#ifdef CT_DEBUG
#define CTDBG(fmt, ...) printf(fmt, ##__VA_ARGS__)
#else
#define CTDBG(fmt, ...) /* fmt */
#endif

#include <net/tcp/tcp_conntrack.hpp>
#include <net/tcp/packet4_view.hpp>

namespace net::tcp {

  static inline Ct_state state(Conntrack::Entry* entry);
  [[maybe_unused]]
  static inline std::string state_str(const uint8_t state);

  static inline void update_timeout(Conntrack::Entry* entry);
  static inline void set_state(Conntrack::Entry* entry, const Ct_state state);
  static inline void close(Conntrack::Entry* entry);

  Conntrack::Entry* tcp4_conntrack(Conntrack& ct, Quadruple q, const PacketIP4& pkt)
  {
    using State = Conntrack::State;
    const auto proto = Protocol::TCP;
    // find the entry
    auto* entry = ct.get(q, Protocol::TCP);

    const auto tcp = Packet4_v(const_cast<PacketIP4*>(&pkt));
    CTDBG("<CT_TCP4> Packet: %s\n", tcp.to_string().c_str());

    // 1. Never seen before, this is hopefully the SYN
    if(UNLIKELY(entry == nullptr))
    {
      // first packet should be SYN
      if(tcp.isset(SYN))
      {
        // this was a SYN, set internal state SYN_SENT & UNREPLIED
        entry = ct.add_entry(q, proto);
        entry->set_flag(Conntrack::Flag::UNREPLIED);
        set_state(entry, Ct_state::SYN_SENT);

        CTDBG("<CT_TCP4> Entry created: %s State: %s\n",
          entry->to_string().c_str(), state_str(entry->other).c_str());
      }

      return entry; // could be nullptr if no SYN
    }

    CTDBG("<CT_TCP4> Entry found: %s State: %s\n",
      entry->to_string().c_str(), state_str(entry->other).c_str());

    // On reset we just have to terminate
    if(UNLIKELY(tcp.isset(RST)))
    {
      close(entry);
      return entry;
    }

    // 2. This is the SYN/ACK reply
    if(UNLIKELY(state(entry) == Ct_state::SYN_SENT and q == entry->second))
    {
      // if SYN/ACK reply
      if(tcp.isset(SYN) and tcp.isset(ACK))
      {
        entry->unset_flag(Conntrack::Flag::UNREPLIED);
        entry->state = State::ESTABLISHED;

        // set internal state SYN_RECV & !UNREPLIED
        set_state(entry, Ct_state::SYN_RECV);
      }

      return entry;
    }

    // 3. ACK to SYN/ACK (handshake done)
    if(UNLIKELY(state(entry) == Ct_state::SYN_RECV and q == entry->first))
    {
      if(tcp.isset(ACK))
      {
        // set internal state ESTABLISHED & ASSURED
        entry->set_flag(Conntrack::Flag::ASSURED);
        set_state(entry, Ct_state::ESTABLISHED);
      }

      return entry;
    }

    // Closing
    if(UNLIKELY(tcp.isset(FIN)))
    {
      switch(state(entry))
      {
        case Ct_state::ESTABLISHED:
          set_state(entry, (q == entry->first) ? Ct_state::FIN_WAIT : Ct_state::CLOSE_WAIT);
          break;
        case Ct_state::FIN_WAIT:
          set_state(entry, Ct_state::TIME_WAIT);
          break;
        case Ct_state::CLOSE_WAIT:
          set_state(entry, Ct_state::LAST_ACK);
          break;
        default:
          CTDBG("FIN in state: %s\n", state_str(entry->other).c_str());
      }
      return entry;
    }

    // Check for ACK when in closing state
    if(UNLIKELY(state(entry) == Ct_state::LAST_ACK or state(entry) == Ct_state::TIME_WAIT))
    {
      // TODO: Every packet has ACK, this should probably check if the ACK is for our FIN.
      // For that we need to know about the TCP state... (sequence number SND.NXT etc..)
      if(tcp.isset(ACK))
      {
        close(entry);
        return entry;
      }
    }

    update_timeout(entry);

    CTDBG("<CT_TCP4> Entry handled: %s State: %s\n",
      entry->to_string().c_str(), state_str(entry->other).c_str());

    return entry;
  }

  static inline Ct_state state(Conntrack::Entry* entry)
  {
    return static_cast<Ct_state>(entry->other);
  }

  static inline void set_state(Conntrack::Entry* entry, const Ct_state state)
  {
    CTDBG("<CT_TCP4> State change %s => %s on %s\n",
      state_str(entry->other).c_str(), state_str((uint8_t)state).c_str(), entry->to_string().c_str());
    reinterpret_cast<Ct_state&>(entry->other) = state;
    update_timeout(entry);
  }

  static inline void close(Conntrack::Entry* entry)
  {
    CTDBG("<CT_TCP4> Closing from state %s\n", state_str(entry->other).c_str());
    set_state(entry, Ct_state::CLOSE);
  }

  static inline void update_timeout(Conntrack::Entry* entry)
  {
    const auto& dur = [&]()->const auto&
    {
      switch(reinterpret_cast<Ct_state&>(entry->other))
      {
        case Ct_state::ESTABLISHED: return timeout::ESTABLISHED;
        case Ct_state::SYN_SENT:    return timeout::SYN_SENT;
        case Ct_state::SYN_RECV:    return timeout::SYN_RECV;
        case Ct_state::FIN_WAIT:    return timeout::FIN_WAIT;
        case Ct_state::TIME_WAIT:   return timeout::TIME_WAIT;
        case Ct_state::CLOSE_WAIT:  return timeout::CLOSE_WAIT;
        case Ct_state::LAST_ACK:    return timeout::LAST_ACK;
        case Ct_state::CLOSE:       return timeout::CLOSE;

        default:                    return timeout::NONE;
      }
    }();

    entry->timeout = RTC::now() + dur.count();
  }

  static inline std::string state_str(const uint8_t state)
  {
    switch(static_cast<Ct_state>(state))
    {
      case Ct_state::SYN_SENT:    return "SYN_SENT";
      case Ct_state::SYN_RECV:    return "SYN_RECV";
      case Ct_state::ESTABLISHED: return "ESTABLISHED";
      case Ct_state::CLOSE:       return "CLOSE";
      case Ct_state::FIN_WAIT:    return "FIN_WAIT";
      case Ct_state::TIME_WAIT:   return "TIME_WAIT";
      case Ct_state::CLOSE_WAIT:  return "CLOSE_WAIT";
      case Ct_state::LAST_ACK:    return "LAST_ACK";
      case Ct_state::NONE:        return "NONE";
      default: return "???";
    }
  }


}
