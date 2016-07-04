#pragma once
#ifndef NET_TCP_COMMON_HPP
#define NET_TCP_COMMON_HPP

#include <net/ip4/ip4.hpp> // IP4::addr

namespace net {
  namespace tcp {

    using Address = IP4::addr;

    /** A port */
    using port_t = uint16_t;

    /** A Sequence number (SYN/ACK) (32 bits) */
    using seq_t = uint32_t;

    /** A shared buffer pointer */
    using buffer_t = std::shared_ptr<uint8_t>;

    class Packet;
    using Packet_ptr = std::shared_ptr<Packet>;

    class Connection;
    using Connection_ptr = std::shared_ptr<Connection>;

  }
}

#endif // < NET_TCP_COMMON_HPP
