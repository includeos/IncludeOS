#ifndef CLASS_TCP_HPP
#define CLASS_TCP_HPP

#include <net/util.hpp> // htons / noths
#include <net/packet.hpp>
#include <net/ip4.hpp>
#include <map>

namespace net {

  /** TCP support. @note Most TCP state logic is implemented inside the TCP::Socket. */
  class TCP{
  public:
    typedef uint16_t port;

    enum Flag {
      NS = (1 << 8),
      CWR = (1 << 7),
      ECE = (1 << 6),
      URG = (1 << 5),
      ACK = (1 << 4),
      PSH = (1 << 3),
      RST = (1 << 2),
      SYN = (1 << 1),
      FIN = 1,
    };

    
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
      
      // Get the raw tcp offset, in quadruples
      inline  uint8_t offset(){ return (uint8_t)(offs_flags.offs_res >> 4); }
      
      // Set raw TCP offset in quadruples
      inline void set_offset(uint8_t offset){ offs_flags.offs_res = (offset << 4); }
    
      // Get tcp header length including options (offset) in bytes
      inline uint8_t size(){ return offset() * 4; }
      
      // Calculate the full header lenght, down to linklayer, in bytes
      uint8_t all_headers_len(){
	return (sizeof(full_header) - sizeof(tcp_header)) + size();
      };
      
      inline void set_flag(Flag f){ offs_flags.whole |= htons(f); }
      inline void set_flags(uint16_t f){ offs_flags.whole |= htons(f); }
      inline void clear_flag(Flag f){ offs_flags.whole &= ~ htons(f); }
      inline void clear_flags(){ offs_flags.whole &= 0x00ff; }
      

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

   
    struct full_header {
      Ethernet::header eth_hdr;
      IP4::ip_header ip_hdr;
      tcp_header tcp_hdr;      
    }__attribute__((packed));

    
    TCP(TCP&) = delete;
    TCP(TCP&&) = delete;
    
    //////////////////////////////////////////////////////////////////
    /** TCP Sockets, implementing most of the TCP state-machine logic. */
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
      inline void ack_keepalive(bool ack){ ack_keepalive_ = ack; }
      
      // Connections (accepted sockets) will be delegated to this kind of handler
      typedef delegate<void(Socket&)> connection_handler;
      
      // This is the default handler
      inline void drop(Socket&){ debug("<Socket::drop> Default handler dropping connection \n"); }
      
      // Our version of "Accept"
      inline void onConnect(connection_handler handler){
	debug("<TCP::Socket> Registered new connection handler \n");
	accept_handler_ = handler;
      }      
      
      void listen(int backlog);      
      
      /** Constructor for server socket */
      Socket(TCP& stack);      
      
      /** Constructor for connections */
      Socket(TCP& local_stack, port local_port, State state);
      
      // IP-stack wiring, analogous to the rest of IncludeOS IP-stack objects
      int bottom(Packet_ptr pckt); 


    private:      
      
      // A private constructor for allowing a listening socket to create connections
      Socket(TCP& local_stack, port local, IP4::addr remote_ip, port remote_port, 
	     State, connection_handler, uint32_t initial_seq_nr_);

      size_t backlog_ = 1000;
      
      // Local end (Local IP is determined by the TCP-object)
      TCP& local_stack_;
      port local_port_;      
      // Remote end
      IP4::addr remote_addr_;
      port remote_port_;
      
      
      // Initial outbound sequence number
      uint32_t initial_seq_out_ = 42;      
      // Initial inbound sequence number
      uint32_t initial_seq_in_ = 0;      


      uint32_t bytes_transmitted_ = 0;
      uint32_t bytes_received_ = 0;
      
      State state_ = CLOSED;
      
      // Assign the "accept-delegate" to the default handler
      connection_handler accept_handler_  = connection_handler::from<Socket,&Socket::drop>(this);
      
      // Respond to keepalive packets?
      bool ack_keepalive_ = false;
      
      // A pretty simple data buffer
      std::string buffer_;
      
      // General ack-function- for syn-ack, fin-ack, ack etc. Pass in the flags you want.
      void ack(Packet_ptr pckt, uint16_t FLAGS = ACK);
      
      // Fill the packet with buffered data. 
      int fill(Packet_ptr pckt);
      
      inline bool is_keepalive(Packet_ptr pckt){
	return tcp_hdr(pckt)->seq_nr == htonl(initial_seq_in_ + bytes_received_ );
      }

      std::shared_ptr<Packet> current_packet_;
      
      // Transmission happens out through TCP& object
      //int transmit(Packet_ptr pckt);
      std::map<std::pair<IP4::addr,port>, Socket > connections;
      
    }; // Socket class end
    //////////////////////////////////////////////////////////////////
    
    Socket& bind(port);
    Socket& connect(IP4::addr, port);
    inline size_t openPorts(){ return listeners.size(); }
    
    /** Delegate output to network layer */
    inline void set_network_out(downstream del)
    { _network_layer_out = del; }
    
    int transmit(Packet_ptr pckt);
  
    int bottom(Packet_ptr pckt);    

    
    TCP(Inet<LinkLayer,IP4>&);
    
  private:
    
    Inet<LinkLayer,IP4>& inet_;
    
    size_t socket_backlog = 1000;
    const IP4::addr& local_ip_;
    
    // For each port on this stack (which has one IP), each IP-Port-Pair represents a connection
    // It's the same as the standard "quadruple", except that local IP is implicit in this TCP-object
    std::map<port, Socket> listeners;
    downstream _network_layer_out;
    
    // Compute the TCP checksum
    uint16_t checksum(Packet_ptr);
            
    // Get the length of actual data in bytes
    static inline uint16_t data_length(Packet_ptr pckt){
      return pckt->size() - tcp_hdr(pckt)->all_headers_len();
    }
    
    // Get the length of the TCP-segment including header and data
    static inline uint16_t tcp_length(Packet_ptr pckt){
      return data_length(pckt) + tcp_hdr(pckt)->size();
    }
    
    // Get the TCP header from a packet
    static inline tcp_header* tcp_hdr(Packet_ptr pckt){
      return &((full_header*)pckt->buffer())->tcp_hdr;
    }
    
    static inline void* data_location(Packet_ptr pckt){
      tcp_header* hdr = tcp_hdr(pckt);
      return (void*)((char*)hdr + hdr->size());
    }
    
    
  };
  
}


#endif
