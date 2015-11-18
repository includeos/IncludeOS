#ifndef NET_IP4_UDP_HPP
#define NET_IP4_UDP_HPP

#include <map>
#include "../ip4.hpp"
#include "../inet.hpp"

namespace net
{
  class PacketUDP;
  class SocketUDP;
  
  /** Basic UDP support. @todo Implement UDP sockets.  */
  class UDP
  {
  public:
    typedef IP4::addr addr_t;
    /** UDP port number */
    typedef uint16_t port;
    
    using Socket = SocketUDP;
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
      Ethernet::header eth_hdr;
      IP4::ip_header ip_hdr;
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
    
    //! construct this UDP module with @inet
    UDP(Stack& inet);
    
  private: 
    downstream _network_layer_out;
    Stack& stack;
    std::map<port, Socket> ports;
    
    friend class SocketUDP;
  };

}

#include "udp.inl"
#include "packet_udp.hpp"
#include "udp_socket.hpp"

#endif
