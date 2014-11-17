#ifndef CLASS_PACKET_H
#define CLASS_PACKET_H

//#include <os>

//#include <net/inet>

//#include <net/class_ethernet.hpp>
//#include <net/class_arp.hpp>
#include <net/class_ip4.hpp>
//#include <net/class_udp.hpp>


namespace net {
  
  class Packet {
  public:
    
    /** Get the buffer */
    const uint8_t* buffer() const
    { return _data; }
    
    /** Get the buffer length */
    inline uint32_t len() const
    { return _len; }

    /** Status of the buffer.
        AVAILABLE : It's just sitting there, free for use
        UPSTREAM : travelling upstream
        DOWNSTREAM : travelling downstream          */
    enum packet_status{ AVAILABLE, UPSTREAM, DOWNSTREAM };
    
    /** Get the packet status */
    packet_status status();
    
    /** Set next-hop ip4. */
    void next_hop(IP4::addr ip);
    
    /** Get next-hop ip4. */
    IP4::addr next_hop();
    
    /** Construct. */
    Packet(uint8_t* data, uint32_t len, packet_status stat);

    /** Destruct. */
    ~Packet();
    
    
    
  private:
    uint8_t* _data;
    uint32_t _len;
    packet_status _status;
    IP4::addr _next_hop4;
    
    
  };
  

  // class Ethernet_Packet : public Packet {      
  // public:    
    
  //   /** Where header ends and "next layer" / data starts */
  //   uint32_t data_start();
      
  //   /** Initialize. Sets the header to point to the right place inside data. */
  //   Ethernet_Packet(Packet p);
    
  // private:
  //   Ethernet::header* hdr = 0;
    
    
  // };
  
  // class IP_Packet : public Packet {
  // public:
  //   /** Where header ends and "next layer" / data starts */
  //   uint32_t data_start();    
  // };
  
  // class IP4_Packet : public Packet {
  // public:
  //   /** Where header ends and "next layer" / data starts */
  //   uint32_t data_start();
    
  //   /** Initialize. Sets the header to point to the right place inside data. */
  //   IP4_Packet(Ethernet_Packet p);
    
  // private:
  //   IP4::ip_header* hdr = 0;
    
    
  // };
  
  
  // class UDP_Packet : public Packet {
    
  // public:
  //   UDP_Packet(IP4_Packet pckt);
  //   UDP::udp_header* hdr = 0;

    
  // };
    

}


#endif
