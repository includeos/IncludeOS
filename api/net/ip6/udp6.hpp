
#ifndef NET_IP6_UDP_HPP
#define NET_IP6_UDP_HPP

#include <deque>
#include <map>
#include <cstring>
#include <unordered_map>

#include "../inet.hpp"
#include "ip6.hpp"
#include <net/packet.hpp>
#include <net/socket.hpp>
#include <util/timer.hpp>
#include <rtc>

namespace net
{
  class PacketUDP6;
  class UDPSocket;

  class UDPv6
  {
  public:
    using addr_t = IP6::addr;
    using port_t = uint16_t;

    using Packet_ptr    = std::unique_ptr<PacketUDP, std::default_delete<net::Packet>>;
    using Stack         = IP6::Stack;

    using Sockets       = std::map<Socket, UDPSocket>;

    typedef delegate<void()> sendto_handler;
    typedef delegate<void(const Error&)> error_handler;
  };
}
#endif
