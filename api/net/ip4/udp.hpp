#ifndef NET_IP4_UDP_HPP
#define NET_IP4_UDP_HPP

#include <map>
#include "../ip4.hpp"

namespace net
{
  class SocketUDP;
  
  /** Basic UDP support. @todo Implement UDP sockets.  */
  class UDP
  {
  public:
  
    /** UDP port number */
    typedef uint16_t port;
  
    /** A protocol buffer temporary type. Later we might encapsulate.*/
    typedef uint8_t* pbuf;
  
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

    /** Input from network layer */
    int bottom(Packet_ptr pckt);
  
    /** Delegate type for listening to UDP ports. */
    typedef upstream listener;
    
    /** Delegate output to network layer */
    inline void set_network_out(downstream del)
    { _network_layer_out = del; }
  
    /** Listen to a port. 
      
        @note Any previous listener will be evicted (it's all your service) */
    void listen(uint16_t port, listener s);

    /** Send UDP datagram from source ip/port to destination ip/port. 
      
        @param sip Source IP address - must be bound to a local interface.
        @param sport source port; replies might come back here
        @param dip Destination IP
        @param dport Destination port   */
    int transmit(Packet_ptr pckt);
  
    UDP();
    
    // the UDP::Socket class
    using Socket = net::SocketUDP;
    friend class net::SocketUDP;
    
    Socket& bind(port port);
    
  private: 
    
    downstream _network_layer_out;
    std::map<uint16_t, listener> ports;
    
    
  };

}

#include "udp_socket.hpp"

#endif
