#ifndef CLASS_TCP_HPP
#define CLASS_TCP_HPP

#include <net/class_ip4.hpp>

namespace net {


  class TCP{
  public:
    typedef uint16_t port;
    
    /** TCP header */    
    struct tcp_header {
      port sport;
      port dport;
      uint32_t seq_nr;
      uint32_t ack_nr;
      union {
	uint16_t whole;
	uint8_t offset;
	uint8_t flags;
      }offs_flags;
      uint16_t win_size;
      uint16_t checksum;
      uint16_t urg_ptr;
      uint32_t options[1]; // 1 to 10 32-bit words  
    };
    
    enum flags {
      NS = 8,
      CWR = 7,
      ECE = 6,
      URG = 5,
      ACK = 4,
      PSH = 3,
      RST = 2,
      SYN = 1,
      FIN = 0,
    };
    
    struct full_header {
      Ethernet::header eth_hdr;
      IP4::ip_header ip_hdr;
      tcp_header tcp_hdr;      
    }__attribute__((packed));
    
    
    /** Delegate output to network layer */
    inline void set_network_out(downstream del)
    { _network_layer_out = del; }
  
    int bottom(std::shared_ptr<Packet>& pckt);    
    int transmit(std::shared_ptr<Packet>& pckt);
    
    TCP();
    
  private:
    downstream _network_layer_out;
    
  };
  
  
}

#endif
