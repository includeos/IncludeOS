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
        
    
    class Socket {
    public:
      enum State {
	CLOSED, LISTEN, SYN_SENT, SYN_RECIEVED, ESTABLISHED, CLOSE_WAIT, LAST_ACK, FIN_WAIT1, FIN_WAIT2,CLOSING,TIME_WAIT
      };
      
      void write(std::string s);
      std::string read(int n=0);
      void close();
      Socket& accept();
      
      void listen(int backlog);
      
      inline State poll(){ return state_; }

      Socket(TCP& stack);

      // IP-stack wiring, analogous to the rest of IncludeOS IP-stack objects
      int bottom(std::shared_ptr<Packet>& pckt); 
      
    private:      
      size_t backlog_ = 1000;
      State state_ = CLOSED;
      
      // Local end (Local IP is determined by the TCP-object)
      port local_port_;      
      TCP& local_stack_;
      
      // Remote end
      IP4::addr dest_;
      port dport_;
            
      //int transmit(std::shared_ptr<Packet>& pckt);

      std::map<std::pair<IP4::addr,port>, Socket > connections;
      
    }; // Socket class end
    
    
    Socket& bind(port);
    Socket& connect(IP4::addr, port);
    
    /** Delegate output to network layer */
    inline void set_network_out(downstream del)
    { _network_layer_out = del; }
  
    int bottom(std::shared_ptr<Packet>& pckt);    
    int transmit(std::shared_ptr<Packet>& pckt);
    
    TCP();
    
  private:
    size_t socket_backlog = 1000;
    // For each port on this stack (which has one IP), each IP-Port-Pair represents a connection
    // It's the same as the standard "quadruple", except that local IP is implicit in this TCP-object
    std::map<port, Socket> listeners;
    downstream _network_layer_out;
    
  };
  
  
}

#endif
