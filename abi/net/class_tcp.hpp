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
      NS = (1 << 8),
      CWR = (1 << 7),
      ECE = (1 << 6),
      URG = (1 << 5),
      ACK = (1 << 4),
      PSH = (1 << 3),
      RST = (1 << 2),
      SYN = (1 << 1),
      FIN = 0,
    };
    
    struct full_header {
      Ethernet::header eth_hdr;
      IP4::ip_header ip_hdr;
      tcp_header tcp_hdr;      
    }__attribute__((packed));

    
    TCP(TCP&) = delete;
    TCP(TCP&&) = delete;
    
    class Socket {
    public:
      enum State {
	CLOSED, LISTEN, SYN_SENT, SYN_RECIEVED, ESTABLISHED, 
	CLOSE_WAIT, LAST_ACK, FIN_WAIT1, FIN_WAIT2,CLOSING,TIME_WAIT
      };
      
      // Common parts
      std::string read(int n=0);
      void write(std::string s);
      void close();
      inline State poll(){ return state_; }
      
      // Server parts
      Socket& accept();     
      void listen(int backlog);      
      
      // Constructor
      Socket(TCP& stack);

      // IP-stack wiring, analogous to the rest of IncludeOS IP-stack objects
      int bottom(std::shared_ptr<Packet>& pckt); 
      
    private:      
      
      // A private constructor for allowing a listening socket to create connections
      Socket(TCP& local_stack, port local_port, IP4::addr rempte_ip, port remote_port, State state);

      size_t backlog_ = 1000;
      State state_ = CLOSED;
      
      // Local end (Local IP is determined by the TCP-object)
      port local_port_;      
      TCP& local_stack_;
      
      // Remote end
      IP4::addr remote_addr_;
      port remote_port_;
      
      void ack(std::shared_ptr<Packet>& pckt); 
      
      // Transmission happens out through TCP& object
      //int transmit(std::shared_ptr<Packet>& pckt);
      std::map<std::pair<IP4::addr,port>, Socket > connections;
      
    }; // Socket class end
    
    
    Socket& bind(port);
    Socket& connect(IP4::addr, port);
    inline size_t openPorts(){ return listeners.size(); }
    
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
