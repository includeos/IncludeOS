// Part of the IncludeOS Unikernel  - www.includeos.org
//
// Copyright 2015 Oslo and Akershus University College of Applied Sciences
// and  Alfred Bratterud. 
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
#include "../ip4.hpp"
#include "../inet.hpp"

namespace net
{
  class PacketUDP;
  template <typename T>
  class Socket;
  
  /** Basic UDP support. @todo Implement UDP sockets.  */
  class UDP
  {
  public:
    typedef IP4::addr addr_t;
    /** UDP port number */
    typedef uint16_t port;
    
    using Socket = Socket<UDP>;
    using Stack  = Inet<LinkLayer,IP4>;
  
    /** UDP header */
    struct udp_header {
      port sport;
      port dport;
      uint16_t length;
      uint16_t checksum;
    };
  
    /** Full UDP Header with all sub-headers */
    struct full_header{
      IP4::full_header full_hdr;
      udp_header udp_hdr;
    }__attribute__((packed));
    
    ////////////////////////////////////////////
    
    inline addr_t local_ip() const
    {
      return stack.ip_addr();
    }
    
    /** Input from network layer */
    int bottom(Packet_ptr pckt);
  
    /** Delegate output to network layer */
    inline void set_network_out(downstream del)
    {
      _network_layer_out = del;
    }
    
    /** Send UDP datagram from source ip/port to destination ip/port. 
      
        @param sip   Local IP-address
        @param sport Local port
        @param dip   Remote IP-address
        @param dport Remote port   */
    int transmit(std::shared_ptr<PacketUDP> udp);
    
    //! @param port local port
    Socket& bind(port port);
    
    //! returns a new UDP socket bound to a random port
    Socket& bind();
    
    //! construct this UDP module with @inet
    UDP(Stack& inet);
    
  private: 
    downstream _network_layer_out;
    Stack& stack;
    std::map<port, Socket> ports;
    port currentPort = 1024;
    
    friend class SocketUDP;
  };

}

#include "udp.inl"
#include "packet_udp.hpp"
#include "udp_socket.hpp"

#endif
