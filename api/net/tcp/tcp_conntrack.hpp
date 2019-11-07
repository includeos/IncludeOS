
#pragma once

#include <net/conntrack.hpp>

namespace net::tcp {

  Conntrack::Entry* tcp4_conntrack(Conntrack& ct, Quadruple q, const PacketIP4& pkt);

  enum class Ct_state : uint8_t
  {
    NONE,
    SYN_SENT,
    SYN_RECV,
    ESTABLISHED,
    FIN_WAIT,
    TIME_WAIT,
    CLOSE_WAIT,
    LAST_ACK,
    CLOSE
  };

  namespace timeout
  {
    static constexpr Conntrack::Timeout_duration ESTABLISHED {24*60*60};
    static constexpr Conntrack::Timeout_duration SYN_SENT    {2*60};
    static constexpr Conntrack::Timeout_duration SYN_RECV    {60};
    static constexpr Conntrack::Timeout_duration FIN_WAIT    {2*60};
    static constexpr Conntrack::Timeout_duration TIME_WAIT   {2*60};
    static constexpr Conntrack::Timeout_duration CLOSE_WAIT  {12*60};
    static constexpr Conntrack::Timeout_duration LAST_ACK    {30};

    static constexpr Conntrack::Timeout_duration NONE        {30*60};
    static constexpr Conntrack::Timeout_duration CLOSE       {10};
  }

}
