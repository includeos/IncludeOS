#define DEBUG
#define DEBUG2

#include <os>
#include <net/class_tcp.hpp>
#include <net/util.hpp>

using namespace net;

TCP::Socket::Socket(TCP& local_stack) : local_stack_(local_stack)
{}

TCP::Socket::Socket(TCP& local_stack, port local_port, State state)
  : local_stack_(local_stack), local_port_(local_port), state_(state)
{
  debug("<Socket::Socket> Constructing socket, local port: %i, State: %i \n",local_port_, state_);
}


TCP::Socket::Socket(TCP& local_stack, port local_port, 
		    IP4::addr remote_ip, port remote_port, State state, connection_handler handler)
  : local_stack_(local_stack), local_port_(local_port), 
    remote_addr_(remote_ip), remote_port_(remote_port), state_(state), accept_handler_(handler)
{
  debug("<Socket::Socket> Constructing CONNECTION, local port: %i , remote IP: %s, remote port: %i\n",
	local_port_, remote_ip.str().c_str(), remote_port);
}


void TCP::Socket::listen(int backlog){
  state_ = LISTEN;
  backlog_ = backlog;
  printf("<TCP::Socket::listen> Listening. (%i)\n",state_);
  // Possibly allocate connectoin pointers (but for now we're using map)  
}


void TCP::Socket::ack(std::shared_ptr<Packet>& pckt){
  
}

void TCP::Socket::syn_ack(std::shared_ptr<Packet>& pckt){
  full_header* full_hdr = (full_header*)pckt->buffer();
  tcp_header* hdr = &(full_hdr)->tcp_hdr;
  
  // Set ack-flag (we assume SYN is allready set)
  auto flags = ntohs(hdr->offs_flags.whole);
  flags |= ACK;
  hdr->offs_flags.whole = htons(flags);
  
  // Set destination ip / port
  auto dest_addr = full_hdr->ip_hdr.saddr;
  auto dest_port = hdr->sport;
  
  debug("<TCP::Socket::syn_ack> Sequence number: %i \n",hdr->seq_nr);
  debug("<TCP::Socket::syn_ack> I've chosen sequence-number: %i and ack-nr. must be %i \n",
	42,ntohs(hdr->seq_nr)+1);
  
  
  // Let's try to modify the same packet, and return it
  hdr->ack_nr = htonl(ntohl(hdr->seq_nr) + 1); // Required
  hdr->seq_nr = 42; //htonl(0xfafafafa); // Random
  
  // Set source port
  hdr->sport = htons(local_port_);  

  // Try to let the underlying stack set the source IP (We don't know it here)
  full_hdr->ip_hdr.saddr.whole = 0; //local_stack_ 
  
  // Set destinatio port, and address
  hdr->dport = dest_port;
  full_hdr->ip_hdr.daddr = dest_addr;
  
  debug("<TCP::Socket::syn_ack> Source port: %i, Destination port: %i (Local port is %i) \n", hdr->sport, hdr->dport, local_port_);
  
  pckt->set_len(sizeof(full_header));
  local_stack_.transmit(pckt);
  
}



std::string TCP::Socket::read(int SIZE){
  return std::string((const char*) data_location(current_packet_), data_length(current_packet_));
}

int TCP::Socket::bottom(std::shared_ptr<Packet>& pckt){
  full_header* full_hdr = (full_header*)pckt->buffer();
  tcp_header* hdr = &(full_hdr)->tcp_hdr;
  auto flags = ntohs(hdr->offs_flags.whole);
  uint8_t offset = (uint8_t)(hdr->offs_flags.offs_res) >> 4;
  auto raw_flags = hdr->offs_flags.flags;
  auto raw_offset = hdr->offs_flags.offs_res;
  
  auto dest_addr = full_hdr->ip_hdr.saddr;
  auto dest_port = hdr->sport;
  auto remote = std::make_pair(dest_addr,dest_port);
  
  
  debug("<TCP::Socket::bottom> State: %i, Source port %i, Dest. port: %i, Flags raw: 0x%x, Flags reversed: 0x%x \n",
	state_, ntohs(hdr->sport), ntohs(hdr->dport),raw_flags,flags);
  
  
  debug("<TCP::Socket::bottom> Raw offset: 0x%x Header size: %i x 4 = %i bytes \n", raw_offset,(int)offset, offset*4);
  
  if (flags & SYN) {       
    
    if (! (flags & ACK)) {
      debug("<TCP::Socket::bottom> SYN-packet; new connection \n");
      assert(state_ == LISTEN);
      
      if (connections.size() >= backlog_) {
	debug("<TCP::Socket::bottom> DROP. We don't have room for more connections.");
	return 0;
      }
      
      // New connection attempt
      // Is it a retransmit?
      if (connections.find(remote) != connections.end()){
	debug("<TCP::Socket::bottom> Remote host allready queued. \n");
	return 0;
      }
      
      // Set up the connection socket
      connections.emplace(remote,
			  TCP::Socket(local_stack_, local_port_, dest_addr, dest_port, SYN_RECIEVED, accept_handler_));
      
      //auto conn = connections[std::make_pair(dest_addr,dest_port)];
      debug("<TCP::Socket::bottom> Connection queued to %s : %i \n",dest_addr.str().c_str(),ntohs(dest_port));
            
      // ACCEPT
      syn_ack(pckt);
      //accept_handler_(connections.at(remote));
      
      return 0;
      
    }else {
      debug("<TCP::Socket::bottom> SYN-ACK; \n");
      assert(state_ == SYN_SENT);
    }    
  }  
  
  if (flags & ACK) {
    debug("<TCP::Socket::bottom> ACK \n");
    // assert(state_ == SYN_RECIEVED);
    
    switch (state_) {
      
    case LISTEN: 
      {
	auto conn_it = connections.find(remote);
	
	if (conn_it == connections.end()){
	  debug("<TCP::Socket::bottom> ACK-packet for non-connected remote host (incomplete handshake) \n");
	  return 0;
	}
	
	(*conn_it).second.bottom(pckt);	
	
	break;
      }
    case SYN_RECIEVED:
      debug("<TCP::Socket::bottom> SYN, SYN-ACK and now ACK. CONNECTED! \n");  
      state_ = ESTABLISHED;            
      // Fall through to established
    case ESTABLISHED:
         
      debug("<TCP::Socket::bottom> CONNECTED ACK. Packet size: %i, Data size: %i \n", pckt->len(), TCP::data_length(pckt));
      
      if (TCP::data_length(pckt)){
	current_packet_ = pckt;
	accept_handler_(*this);
	
	ack(pckt);
      }
      
    }
    
  }
  
  return 0;
}
