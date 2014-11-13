#ifndef CLASS_ICMP_HPP
#define CLASS_ICMP_HPP

namespace net {

  class ICMP{
  
  public:

    struct icmp_header {
      uint8_t type;
      uint8_t code;
      uint16_t checksum;
      uint32_t rest;
    }__attribute__((packed));
    
    struct full_header {
      Ethernet::header eth_hdr;
      IP4::ip_header ip_hdr;
      icmp_header icmp_hdr;
    }__attribute__((packed));

    /** Known ICMP types */
    enum icmp_types {ICMP_ECHO_REPLY,ICMP_ECHO = 8};
  
    /** Input from network layer */
    int bottom(std::shared_ptr<Packet> pckt);
  
    /** Delegate output to network layer */
    inline void set_network_out(downstream s)
    { _network_layer_out = s;  };
  
    /** Initialize. Sets a dummy handler for network layer out.*/
    ICMP();
  
  private: 
  
    //TEMP Reply buffer pointer
    uint8_t* buf = 0;

    downstream _network_layer_out;
    void ping_reply(full_header* full_hdr);
  
  };

} // ~net
#endif
