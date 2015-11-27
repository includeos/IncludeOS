#ifndef NET_IP4_ICMP_HPP
#define NET_IP4_ICMP_HPP

namespace net {
  
  int icmp_default_out(Packet_ptr);
  
  class ICMP{
  
  public:

    struct icmp_header {
      uint8_t type;
      uint8_t code;
      uint16_t checksum;
      uint16_t identifier;
      uint16_t sequence;
      uint8_t  payload[0];
    }__attribute__((packed));
    
    struct full_header {
      LinkLayer::header link_hdr;
      IP4::ip_header ip_hdr;
      icmp_header icmp_hdr;
    }__attribute__((packed));

    /** Known ICMP types */
    enum icmp_types {ICMP_ECHO_REPLY,ICMP_ECHO = 8};
  
    /** Input from network layer */
    int bottom(Packet_ptr pckt);
  
    /** Delegate output to network layer */
    inline void set_network_out(downstream s)
    { _network_layer_out = s;  };
  
    /** Initialize. 
     */
    ICMP(Inet<LinkLayer,IP4>& inet);
  
  private: 
    
    Inet<LinkLayer,IP4>& inet_;
    
    downstream _network_layer_out = icmp_default_out;
    void ping_reply(full_header* full_hdr, uint16_t size);
  
  };

} // ~net
#endif
