#ifndef CLASS_TCP_HPP
#define CLASS_TCP_HPP

#include <net/class_ip4.hpp>

#define DEBUG 1

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
	struct{
	  uint8_t offs_res;
	  uint8_t flags;
	};
      }offs_flags;
      uint16_t win_size;
      uint16_t checksum;
      uint16_t urg_ptr;
      uint32_t options[0]; // 0 to 10 32-bit words  
    }__attribute__((packed));

    /** TCP Pseudo header, for checksum calculation */
    struct pseudo_header{
      IP4::addr saddr;
      IP4::addr daddr;
      uint8_t zero;
      uint8_t proto;
      uint16_t tcp_length;      
    }__attribute__((packed));
    
    /** TCP Checksum-header (TCP-header + pseudo-header */    
    struct checksum_header{
      pseudo_header pseudo_hdr;
      tcp_header tcp_hdr;
    }__attribute__((packed));

    
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
      // Posix-style accept doesn't really make sense here, as we don't block
      // Socket& accept();     
      
      // Connections (accepted sockets) will be delegated to this kind of handler
      typedef delegate<void(Socket&)> connection_handler;
      
      // This is the default handler
      inline void drop(Socket&){ debug("<Socket::drop> Default handler dropping connection \n"); }
      
      // Assign the "accept-delegate" to the default handler
      connection_handler accept_handler_  = connection_handler::from<Socket,&Socket::drop>(this);
      
      // Our version of "Accept"
      inline void onConnect(connection_handler handler){
	debug("<TCP::Socket> Registered new connection handler \n");
	accept_handler_ = handler;
      }      
      
      void listen(int backlog);      
      
      // Constructor
      Socket(TCP& stack);      
      Socket(TCP& local_stack, port local_port, State state);
      
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
      void syn_ack(std::shared_ptr<Packet>& pckt); 
      
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
    
    int transmit(std::shared_ptr<Packet>& pckt);
  
    int bottom(std::shared_ptr<Packet>& pckt);    

    
    TCP(IP4::addr);
    
  private:
    size_t socket_backlog = 1000;
    IP4::addr local_ip_;
    // For each port on this stack (which has one IP), each IP-Port-Pair represents a connection
    // It's the same as the standard "quadruple", except that local IP is implicit in this TCP-object
    std::map<port, Socket> listeners;
    downstream _network_layer_out;
    uint16_t checksum(std::shared_ptr<net::Packet>&);
    
    static void set_offset(tcp_header* hdr, uint8_t offset);
    static uint8_t get_offset(tcp_header* hdr);
    static uint8_t header_len(tcp_header* hdr);
    
  };
  
  
}

#endif
