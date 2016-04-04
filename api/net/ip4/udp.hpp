// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015 Oslo and Akershus University College of Applied Sciences
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

#ifndef NET_IP4_UDP_HPP
#define NET_IP4_UDP_HPP

#include <map>

#include "../inet.hpp"
#include "ip4.hpp"

namespace net {

  class PacketUDP;

  template <typename T>
  class Socket;

  void ignore_udp(Packet_ptr);

  /** Basic UDP support. @todo Implement UDP sockets.  */
  class UDP {
  public:
    using addr_t = IP4::addr;

    /** UDP port number */
    using port_t = uint16_t;
  
    using Socket = Socket<UDP>;
    using Stack  = Inet<LinkLayer, IP4>;

    /** UDP header */
    struct udp_header {
      port_t   sport;
      port_t   dport;
      uint16_t length;
      uint16_t checksum;
    };

    /** Full UDP Header with all sub-headers */
    struct full_header {
      IP4::full_header full_hdr;
      udp_header       udp_hdr;
    }__attribute__((packed));
  
    ////////////////////////////////////////////
  
    inline addr_t local_ip() const
    { return stack_.ip_addr(); }
  
    /** Input from network layer */
    void bottom(Packet_ptr);

    /** Delegate output to network layer */
    inline void set_network_out(downstream del)
    { network_layer_out_ = del; }
  
    /** Send UDP datagram from source ip/port to destination ip/port. 
    
        @param sip   Local IP-address
        @param sport Local port
        @param dip   Remote IP-address
        @param dport Remote port   */
    void transmit(std::shared_ptr<PacketUDP> udp);
  
    //! @param port local port
    Socket& bind(port_t port);
  
    //! returns a new UDP socket bound to a random port
    Socket& bind();
  
    //! construct this UDP module with @inet
    UDP(Stack& inet) :
      network_layer_out_ {ignore_udp},
      stack_ {inet}
    { }
  private: 
    downstream               network_layer_out_;
    Stack&                   stack_;
    std::map<port_t, Socket> ports_;
    port_t                   current_port_ {1024};
  
    friend class SocketUDP;
  }; //< class UDP
} //< namespace net

#include "packet_udp.hpp"
#include "udp_socket.hpp"

#endif
