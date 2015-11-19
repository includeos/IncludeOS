#define DEBUG
#define DEBUG2

#include <os>
#include <net/tcp.hpp>
#include <net/util.hpp>
#include <net/packet.hpp>

using namespace net;

TCP::Socket::Socket(TCP& local_stack) : local_stack_(local_stack)
{}

TCP::Socket::Socket(TCP& local_stack, port local_port, State state)
  : local_stack_(local_stack), local_port_(local_port), state_(state)
{
  debug2("<Socket::Socket> Constructing socket, local port: %i, State: %i \n",local_port_, state_);
}


TCP::Socket::Socket(TCP& local_stack, port local_port, 
		    IP4::addr remote_ip, port remote_port, 
		    State state, connection_handler handler, uint32_t initial_seq)
  : local_stack_(local_stack), local_port_(local_port), 
    remote_addr_(remote_ip), remote_port_(remote_port), 
    initial_seq_in_(initial_seq), state_(state), accept_handler_(handler)
{
  debug2("<Socket::Socket> Constructing CONNECTION, local port: %i , remote IP: %s, remote port: %i\n",
	local_port_, remote_ip.str().c_str(), remote_port);
}


void TCP::Socket::listen(int backlog){
  state_ = LISTEN;
  backlog_ = backlog;
  debug2("<TCP::Socket::listen> Listening. (%i)\n",state_);
  // Possibly allocate connectoin pointers (but for now we're using map)  
}

void TCP::Socket::ack(Packet_ptr pckt, uint16_t FLAGS){
  full_header* full_hdr = (full_header*)pckt->buffer();
  tcp_header* hdr = &(full_hdr)->tcp_hdr;
  
  // Reset flags
  hdr->clear_flags();
  hdr->set_flags(FLAGS);
  
  // Set destination ip / port
  auto dest_addr = full_hdr->ip_hdr.saddr;
  auto dest_port = hdr->sport;
  
  #ifdef DEBUG
  auto expected_seq_nr_ = hdr->ack_nr;  
  #endif 
  auto incoming_seq_nr_ = hdr->seq_nr;
  
  // WARNING: We're currently just modifying the same packet, and returning it
 
  // Set sequence number
  if (FLAGS & SYN)
    hdr->seq_nr = htonl(initial_seq_out_); //htonl(0xfafafafa); // Random  
  else if (bytes_transmitted_ == 0) 
      hdr->seq_nr = hdr->seq_nr = htonl(initial_seq_out_ + 1);
  else
    hdr->seq_nr = hdr->seq_nr = htonl(initial_seq_out_ + bytes_transmitted_ + 1);
  
  // Set ack-nr. 
  if (bytes_received_ == 0) {
    hdr->ack_nr = htonl(ntohl(incoming_seq_nr_) + 1); 
  }else {
    hdr->ack_nr = htonl(initial_seq_in_ + bytes_received_ + 1);
    debug2("\t Bytes received: %u, Ack-nr.: %u, htonl(ack_nr): %u, Ack-nr. on wire: %u, ntohl(wire): %u\n", 
	   bytes_received_, bytes_received_ + 1, htonl(bytes_received_ +1), hdr->ack_nr, ntohl(hdr->ack_nr));
  }
  
  
  debug("<TCP::Socket::syn_ack> OUT: %sACK: Seq_nr: %u, Ack-nr.: %u FLAGS: 0x%x\n",
	FLAGS & SYN ? "SYN-" : "",ntohl(hdr->seq_nr), ntohl(hdr->ack_nr), FLAGS);
  
  debug("\t %sEXPECTED sequence number %s ACTUAL\n", 
	expected_seq_nr_ != hdr->seq_nr && ! (FLAGS & SYN) ? "WARNING: ":"", 
	expected_seq_nr_ != hdr->seq_nr && ! (FLAGS & SYN) ? " DIFFERS FROM " : "EQUALS");
  
  
  // Set source port
  hdr->sport = htons(local_port_);  
  
  // Try to let the underlying stack set the source IP (We don't know it here)
  full_hdr->ip_hdr.saddr.whole = 0; //local_stack_ 
  
  // Set destinatio port, and address
  hdr->dport = dest_port;
  full_hdr->ip_hdr.daddr = dest_addr;
  
  debug2("<TCP::Socket::syn_ack> Source port: %i, Destination port: %i (Local port is %i) \n", 
	 hdr->sport, hdr->dport, local_port_);
  
  
  // Shrink-wrap the packet around the header
  pckt->set_size(sizeof(full_header));

  // Fill up with data from the buffer
  if (buffer_.size())
    fill(pckt);
  
  local_stack_.transmit(pckt);
  
}

std::string TCP::Socket::read(int SIZE){
  (void) SIZE;
  return std::string((const char*) data_location(current_packet_), data_length(current_packet_));
}


void TCP::Socket::write(std::string data){
  // Just buffer up the data and let the state-machine (i.e. void ack()) decide when it goes out.
  buffer_ += data;
}

int TCP::Socket::fill(Packet_ptr pckt){
  size_t capacity = pckt->capacity() - pckt->size();
  void* out_buf = (char*) data_location(pckt);
  size_t bytecount = capacity > buffer_.size() ? buffer_.size() : capacity;
  
  // Copy the data
  memcpy(out_buf, (void*)buffer_.data(), bytecount);
  
  // Shrink the buffer, update packet-length and transmitted byte count
  buffer_.resize(buffer_.size() - bytecount);
  bytes_transmitted_ += bytecount;  
  pckt->set_size(pckt->size() + bytecount);
  
  debug("<TCP::Socket::fill> FILLING packet with %i bytes \n", bytecount);
  return bytecount;
  
};

int TCP::Socket::bottom(Packet_ptr pckt){
  full_header* full_hdr = (full_header*)pckt->buffer();
  tcp_header* hdr = &(full_hdr)->tcp_hdr;
  auto flags = ntohs(hdr->offs_flags.whole);
  //uint8_t offset = (uint8_t)(hdr->offs_flags.offs_res) >> 4;
  
  #ifdef DEBUG2
  auto raw_flags = hdr->offs_flags.flags;
  //auto raw_offset = hdr->offs_flags.offs_res;
  #endif
  
  auto dest_addr = full_hdr->ip_hdr.saddr;
  auto dest_port = hdr->sport;
  auto remote = std::make_pair(dest_addr,dest_port);
  
  debug2("<TCP::Socket::bottom> State: %i, Source port %i, Dest. port: %i, Flags raw: 0x%x, Flags reversed: 0x%x \n", state_, ntohs(hdr->sport), ntohs(hdr->dport),raw_flags,flags);
  
  
  debug2("<TCP::Socket::bottom> IN: Seq_nr: %u, Ack-number: %u  (little-endian) Packet size: %i \n", 
	 ntohl(hdr->seq_nr), ntohl(hdr->ack_nr), pckt->size());
  
  switch (state_) {
    
  case LISTEN: 
    {
      // SYN
      if (flags & SYN) {       		
	// SYN-ACK in listening state doesn't make sense
	if (flags & ACK) {
	  debug2("TCP::Socket::bottom> WARNING: Ignored SYN-ACK packet. I'm in LISTENing state \n");
	  return 0;
	}
	
	debug("<TCP::Socket::bottom> IN (State 1) SYN: Seq_nr: %u, Ack_nr: %u (little-endian) \n", 
	      ntohl(hdr->seq_nr), ntohl(hdr->ack_nr));
	
	// We need to limit the number of connections (From the standard)
	if (connections.size() >= backlog_) {
	  debug("<TCP::Socket::bottom> DROP. We don't have room for more connections.");
	  return 0;
	}
	
	// New connection attempt
	// Is it a retransmitted syn-packet?
	if (connections.find(remote) != connections.end()){
	  debug("<TCP::Socket::bottom> Remote host allready queued. \n");
	  return 0;
	}
	
	// Set up the connection socket
	connections.emplace(remote, TCP::Socket(local_stack_, local_port_, 
						dest_addr, dest_port, 
						SYN_RECIEVED, accept_handler_, ntohl(hdr->seq_nr)));
	
	// ACCEPT
	ack(pckt, SYN | ACK ); 
	return 0;
      }  
            
      // Find a connection for the packet
      auto conn_it = connections.find(remote);      
      if (conn_it == connections.end()){
	debug("<TCP::Socket::bottom> ACK-packet for non-connected remote host (incomplete handshake) \n");
	return 0;
      }
      
      // Pass the packet up to the connection
      (*conn_it).second.bottom(pckt);	      
      break;
    }
  
  case SYN_RECIEVED:
    
    debug("<TCP::Socket::bottom> IN (State 2): ACK - CONNECTED! \n");  
    state_ = ESTABLISHED;                
    
    // We're not going to ack the last ack of the handshake
    break;
    
  case ESTABLISHED:
    {
      auto data_size = TCP::data_length(pckt);
      debug("<TCP::Socket::bottom> IN (State 3): %s ACK. Packet size: %i, Data size: %i, seq_nr: %u, ack_nr: %u \n", 
	    flags & FIN ? "FIN" : "", pckt->size(), data_size, ntohl(tcp_hdr(pckt)->seq_nr), ntohl(tcp_hdr(pckt)->ack_nr));
      
      if (data_size){
	bytes_received_ += data_size;
	current_packet_ = pckt;	
	
	// Call application. Might result in a close().
	accept_handler_(*this);
      }
      
      // PASSIVE CLOSE:
      // He wants to close the connection. 
      if (flags & FIN){
	
	if (! data_size)
	  bytes_received_++;
	
	state_ = CLOSE_WAIT;
	// @TODO: Warn the application		
	// if (buffer_.size() > pckt.capacity()) ... flush
	
	// Close
	ack(pckt, ACK);
	state_ = LAST_ACK;
	return 0;
      
      } 
      
      // If no data, this is (most likely) a keepalive-ack
      if (is_keepalive(pckt)){
	debug ("\tKEEPALIVE %s \n",ack_keepalive_ ? "Ack." : "DROP");
	
	if (! ack_keepalive_) return 0;
      }
      
      // Ack
      ack(pckt); 
      
      
      break;
      
    }
    
  case LAST_ACK:
    {
      if (flags & ACK) {
	state_ = CLOSED;
	debug("Connection CLOSED. Please clean up somehow \n");
      }else{
	debug("WARNING: LAST_ACK received a non-ack packet. Flags: %i \n", flags);
      }
      break;
    }
    
  default:
    break;
    // Don't think we have to ack every packet 
  }
  return 0;
}
