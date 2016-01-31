// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015-2016 Oslo and Akershus University College of Applied Sciences
// and Alfred Bratterud
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
//     http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef NET_TCP_HPP
#define NET_TCP_HPP

#include <net/ip4.hpp> // IP4::Addr
#include <net/ip4/packet_ip4.hpp> // PacketIP4
#include <net/util.hpp> // net::Packet_ptr, htons / noths
#include <map>

namespace net {

class TCP {
private:
	using Address = IP4::addr;
	using Port = uint16_t;
	/*
		A Sequence number (SYN/ACK) (32 bits)
	*/
	using Seq = uint32_t;

	using IPStack = Inet<LinkLayer,IP4>;

public:
	/*
		An IP address and a Port.
	*/
	class Socket {
	private:
		using Address = IP4::addr;
		
	public:
		/*
			Intialize an empty socket.
		*/
		inline Socket() : address_(0), port_(0) {};

		/*
			Create a socket with a Address and Port.
		*/
		inline Socket(Address address, TCP::Port port) : address_(address), port_(port) {};

		/*
			Returns the Socket's address.
		*/
		inline const Socket::Address& address() const { return address_; }

		/*
			Returns the Socket's port.
		*/
		inline const TCP::Port& port() const { return port_; }

		/*
			Returns a string in the format "Address:Port".
		*/
		std::string to_string() const {
			std::ostringstream os;
			os << address_ << ":" << port_;
			return os.str();
		}

		inline static bool is_empty(const Socket& socket) {
			return (socket.address() == 0 and socket.port()) == 0;
		}

		inline bool is_empty() const { return (address_ == 0 and port_ == 0); }

		/*
			TODO: Add compare for use in map/vector.
		*/

	private:
		//SocketID id_; // Maybe a hash or something. Not sure if needed (yet)
		Address address_;
		TCP::Port port_;

	}; // << class TCP::Socket


	/////// TCP Stuff - Relevant to the protocol /////
	
	static constexpr uint16_t default_window_size = 0xffff;

	/* 
		Flags (Control bits) in the TCP Header.
	*/
	enum Flag {
		NS 		= (1 << 8), 	// Nounce (Experimental: see RFC 3540)
		CWR 	= (1 << 7),		// Congestion Window Reduced
		ECE 	= (1 << 6),		// ECN-Echo
		URG 	= (1 << 5),		// Urgent
		ACK 	= (1 << 4),		// Acknowledgement
		PSH 	= (1 << 3),		// Push
		RST 	= (1 << 2),		// Reset
		SYN 	= (1 << 1),		// Syn(chronize)
		FIN 	= 1,			// Fin
    };

    /*
		Representation of the TCP Header.

		RFC 793, (p.15):
	    0                   1                   2                   3   
	    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 
	   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	   |          Source Port          |       Destination Port        |
	   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	   |                        Sequence Number                        |
	   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	   |                    Acknowledgment Number                      |
	   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	   |  Data |           |U|A|P|R|S|F|                               |
	   | Offset| Reserved  |R|C|S|S|Y|I|            Window             |
	   |       |           |G|K|H|T|N|N|                               |
	   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	   |           Checksum            |         Urgent Pointer        |
	   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	   |                    Options                    |    Padding    |
	   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	   |                             data                              |
	   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    */
	struct Header {
		TCP::Port source_port;			// Source port
		TCP::Port destination_port;		// Destination port
		uint32_t seq_nr;				// Sequence number
		uint32_t ack_nr;				// Acknowledge number
		union {
			uint16_t whole;				// Reference to offset_reserved & flags together.
			struct {
			  	uint8_t offset_reserved;	// Offset (4 bits) + Reserved (3 bits) + NS (1 bit)
			  	uint8_t flags;				// All Flags (Control bits) except NS (9 bits - 1 bit)
			};
      	} offset_flags;					// Data offset + Reserved + Flags (16 bits)
      	uint16_t window_size;			// Window size
      	uint16_t checksum;				// Checksum
      	uint16_t urgent;				// Urgent pointer offset
      	uint32_t options[0];			// Options

      	// Get the raw tcp offset, in quadruples
		inline uint8_t offset() { return (uint8_t)(offs_flags.offset_reserved >> 4); }

		// Set raw TCP offset in quadruples
		inline void set_offset(uint8_t offset){ offs_flags.offset_reserved = (offset << 4); }

		// Get tcp header length including options (offset) in bytes
		inline uint8_t size() { return offset() * 4; }

		// Calculate the full header lenght, down to linklayer, in bytes
		uint8_t all_headers_len(){
		return (sizeof(TCP::Full_header) - sizeof(TCP::Header)) + size();
		};

		inline void set_flag(Flag f){ offset_flags.whole |= htons(f); }
		inline void set_flags(uint16_t f){ offs_flags.whole |= htons(f); }
		inline void clear_flag(Flag f){ offset_flags.whole &= ~ htons(f); }
		inline void clear_flags(){ offset_flags.whole &= 0x00ff; }
	}__attribute__((packed)); // << struct TCP::Header


	/* 
		TCP Pseudo header, for checksum calculation
	*/
	struct Pseudo_header {
		IP4::addr saddr;
		IP4::addr daddr;
		uint8_t zero;
		uint8_t proto;
		uint16_t tcp_length;
	}__attribute__((packed));
    
	/*
		TCP Checksum-header (TCP-header + pseudo-header
	*/
	struct Checksum_header {
		TCP::Pseudo_header pseudo;
		TCP::Header tcp;
	}__attribute__((packed));

	/*
		To extract the TCP part from a Packet_ptr and calculate size. (?)
	*/
	struct Full_header {
		Ethernet::header ethernet;
		IP4::ip_header ip4;
		TCP::Header tcp; 
	}__attribute__((packed));


	/*
		A Wrapper for a TCP Packet. Is everything as a IP4 Packet, 
		in addition to the TCP Header and functions to modify this and the control bits (FLAGS).
	*/
	class Packet : public PacketIP4 {
	public:
    
	    inline TCP::Header& header() const
	    {
	     	return ((TCP::Full_header*) buffer())->tcp;
	    }
    
    	static const size_t HEADERS_SIZE = sizeof(TCP::Full_header);
    
	    //! initializes to a default, empty TCP packet, given
	    //! a valid MTU-sized buffer
    	void init()
    	{            
	      // Erase all headers (smart? necessary? ...well, convenient)
    		memset(buffer(), 0, sizeof(TCP::Full_header));
    		PacketIP4::init();

    		set_protocol(IP4::IP4_TCP);
    		set_win_size(TCP::default_window_size);

	      	// set TCP payload location (!?)
    		payload_ = buffer() + sizeof(TCP::Full_header);
    	}


    	inline TCP::Port src_port() const
    	{
    		return ntohs(header().source_port);
    	}

    	inline TCP::Port dst_port() const
    	{
    		return ntohs(header().destination_port);
    	}

    	inline TCP::Seq seq() const {
    		return header().seq_nr;
    	}

    	inline TCP::Seq ack() const {
    		return header().seq_nr;
    	}

    	inline TCP::Packet& set_src_port(TCP::Port p) { 
    		header().source_port = htons(p);
    		return *this;
    	}

    	inline TCP::Packet& set_dst_port(TCP::Port p) { 
    		header().destination_port = htons(p);
    		return *this;
    	}

    	inline TCP::Packet& set_seq(TCP::Seq n) { 
    		header().seq_nr = htonl(n);
    		return *this;
    	}

    	inline TCP::Packet& set_ack(TCP::Seq n) { 
    		header().ack_nr = htonl(n);
    		return *this;
    	}

    	inline TCP::Packet& set_flag(TCP::Flag f) { 
    		header().set_flag(f);
    		return *this;
    	}

    	inline TCP::Packet& set_flags(uint16_t f) { 
    		header().set_flags(f);
    		return *this;
    	}

    	inline TCP::Packet& set_win_size(uint16_t size) { 
    		header().window_size = htons(size);
    		return *this;
    	}

    	inline bool isset(TCP::Flag f)
    	{ return ntohs(header().offset_flags.whole) & f; }

    	inline uint16_t data_length() const
    	{      
    		return size() - header().all_headers_len();
    	}

    	inline uint16_t tcp_length() const 
    	{
    		return data_length() + header().size();
    	}

    	inline char* data()
    	{
    		return (char*) (buffer() + sizeof(TCP::Full_header));
    	}

    	// sets the correct length for all the protocols up to IP4
    	void set_length(uint16_t newlen)
    	{
     		// new total packet length
    		set_size( sizeof(TCP::Full_header) + newlen );
    	}
    
    	// generates a new checksum and sets it for this TCP packet
    	uint16_t gen_checksum() { 
    		header().checksum =TCP::checksum(Packet::shared_from_this());
    		return header().checksum;
    	}

    	inline TCP::Packet& set_checksum(uint16_t checksum) {
    		header().checksum = checksum;
    		return *this;
    	}

	    //! assuming the packet has been properly initialized,
	    //! this will fill bytes from @buffer into this packets buffer,
	    //! then return the number of bytes written. buffer is unmodified
    	uint32_t fill(const std::string& buffer)
    	{
    		uint32_t rem = capacity();
    		uint32_t total = (buffer.size() < rem) ? buffer.size() : rem;
      		// copy from buffer to packet buffer
    		memcpy(data() + data_length(), buffer.data(), total);
      		// set new packet length
    		set_length(data_length() + total);
    		return total;
    	}

    	inline TCP::Socket& source() const {
    		return {src(), src_port()};
    	}

    	inline TCP::Packet& set_source(const TCP::Socket& src) {
    		set_src(src.address()); // PacketIP4::set_src
    		header()->source_port = src.port();
    		return *this;
    	}

    	inline TCP::Socket& destination() const {
    		return {dst(), dst_port()};
    	}

    	inline TCP::Packet& set_destination(const TCP::Socket& dest) {
    		set_dst(dest.address()); // PacketIP4::set_dst
    		header()->destination_port = dest.port();
    		return *this;
    	}
	}; // << class TCP::Packet

	// Maybe this is a very stupid alias....
	using Packet_ptr = std::shared_ptr<TCP::Packet>;
	// Even more stupid? => TCP::Packet::ptr


	/*
		A connection between two Sockets (local and remote).
		Receives and handle TCP::Packet.
		Transist between many states.
	*/
	class Connection {
	public:
		struct Tuple {
			Socket& local;
			Socket& remote;

			// @TODO: Add compare for lookup in map/vector.
		};
		/*
			Creates a connection without a remote.
		*/
		Connection(TCP& host, Socket& local, TCP::Seq iss);

		/*
			Creates a connection with a remote.
		*/
		Connection(TCP& host, Socket& local, Socket&& remote, TCP::Seq iss);

		/*
			The local Socket bound to this connection.
		*/
		inline TCP::Socket& local() const { return local_; }

		/*
			The remote Socket bound to this connection.
		*/
		inline TCP::Socket& remote() const { return remote_; }

		/*
			Set remote Socket bound to this connection.
		*/
		inline void set_remote(Socket&& remote) { remote_ = remote; }

		/*
			Read content from remote.
		*/
		std::string read(uint16_t buffer_size);

		/*
			Write content to remote.
		*/
		void write(std::string data);

		/*
			Open connection.
		*/
		void open(bool active = false);

		/*
			Callback for success
		*/
		inline Connection& onSuccess(SuccessCallback callback) { on_success_handler_ = callback; }

		/*
			Callback for close()
		*/
		//Connection& onClose(SuccessCallback callback);

		/*
			Callback for errors.
		*/
		//Connection& onError(ErrorCallback callback);

		/*
			The same as status()
		*/
		std::string to_string() const;

		/*
			Returns the current state of the connection.
		*/
		Connection::State& state() const { return state_; }

		/*
			Returns the control_block.

			@WARNING: Public, for use in sub-state.
		*/
		inline Connection::TCB& tcb() const { return control_block; }

		/*
			Return the id (TUPLE) of the connection.
		*/
		Connection::Tuple tuple();

		/*
			Receive a TCP Packet.

			@WARNING: Public, for use in TCP::bottom
		*/
		void receive(Packet&);

		/*
			Destroy the Connection.

			Clean up.
		*/
		~Connection();

	private:
		TCP::Socket& local_;
		TCP::Socket remote_;

		TCP& host_;

		SuccessCallback on_success_handler_;
		/*
			Transmission Control Block.
			Keep tracks of all the data for a connection.
			
			Note:
			What I can read from the RFC, the TCB **is** what I am calling "Connection".
			Question is if the struct TCB is needed, or if Connection should be renamed TCB, 
			and expose the interface Connection to users.

			RFC 793: Page 19
			Among the variables stored in the
  			TCB are the local and remote socket numbers, the security and
  			precedence of the connection, pointers to the user's send and receive
  			buffers, pointers to the retransmit queue and to the current segment.
  			In addition several variables relating to the send and receive
  			sequence numbers are stored in the TCB.
		*/
		struct TCB {
			/* Send Sequence Variables */
			struct {
				TCP::Seq UNA;	// send unacknowledged
				TCP::Seq NXT;	// send next
				uint16_t WND;	// send window
				uint16_t UP;	// send urgent pointer
				TCP::Seq WL1;	// segment sequence number used for last window update
				TCP::Seq WL2;	// segment acknowledgment number used for last window update
			} SND; // <<
			TCP::Seq ISS;		// initial send sequence number

			/* Receive Sequence Variables */
			struct {
				TCP::Seq NXT;	// receive next
				uint16_t WND;	// receive window
				uint16_t UP;	// receive urgent pointer
			} RCV; // <<
			TCP::Seq IRS;		// initial receive sequence number

			TCB() : SND.UNA(0), SND.NXT(0), SND.WND(0), SND.UP(0), SND.WL1(0), SND.WL2(0), ISS(0),
					RCV.NXT(0), RCV.UP(0), IRS(0) {};
		} control_block; // < struct TCP::Connection::TCB

		/*
			Interface for one of the many states a Connection can have.
		*/
		class State {
		protected:
			using Packet = TCP::Packet;
		public:
			/*
				Open a Connection.
				OPEN

				@TODO: Add support for PASSIVE/OPEN
			*/
			virtual void open(Connection&);

			/*
				Write to a Connection.
				SEND
			*/
			//virtual void send(Connection&);

			/*
				Read from a Connection.
				RECEIVE
			*/
			//virtual void receive(Connection&);
			
			/*
				Close a Connection.
				CLOSE
			*/
			virtual void close(Connection&);
			
			/*
				Handle a Packet
				SEGMENT ARRIVES
			*/
			virtual int handle(Connection&, TCP::Packet_ptr);

			/*
				The current state represented as a string.
				STATUS
			*/
			virtual std::string to_string() const;

		protected:		
			/*
				Set the state on the Connection.
			*/
			virtual void set_state(Connection&, State&);

		}; // < class TCP::Connection::State

		//friend State; // No need for friendship when inner class(?)

		State& state_;

		/*
			Transmit outgoing packet.
		*/
		void transmit(TCP::Packet_ptr);

		/*
			Return a new outgoing packet.
		*/
		TCP::Packet_ptr create_outgoing_packet();

	}; // < class TCP::Connection


	////// USER INTERFACE - TCP //////
	/*
		Callback for Connection
	*/
	using ConnectionCallback = Connection::SuccessCallback;
	using ConnectionErrorCallback = Connection::ErrorCallback;

	/*
		Constructor
	*/
	TCP(IPStack&);
	
	/*
		Bind a new connection to a given Port.	
	*/
	Connection& bind(Port port);

	/*
		Bind a new connection to a given Port with a Callback.
	*/
	void bind(Port port, Connection::SuccessCallback);

	/*
		Active open a new connection to the given remote.
	*/
	Connection& connect(Socket& remote);

	/*
		Active open a new connection to the given remote.
	*/
	void connect(Socket& remote, Connection::SuccessCallback);

	/*
		Receive packet from network layer (IP).
	*/
	void bottom(net::Packet_ptr);

	/* 
		Delegate output to network layer
	*/
	inline void set_network_out(downstream del) { _network_layer_out = del; }

	/*
		Compute the TCP checksum
	*/
    static uint16_t checksum(const TCP::Packet_ptr);

	/*
		Show all connections for TCP as a string.
	*/
	std::string status() const;

	/*
		Translate net::Packet_ptr to TCP::Packet_ptr
		TODO: Come up with a real name..
	*/
	static TCP::Packet_ptr net2tcp(net::Packet_ptr);

private:
	IPStack& inet_;
	std::vector<Socket&> listeners;
	std::map<Connection::Tuple, Connection> connections;

	downstream _network_layer_out;
	
	/*
		Transmit packet to network layer (IP).
	*/
	void transmit(TCP::Packet_ptr);

	/*
		Generate a unique initial sequence number (ISS).
	*/
	TCP::Seq generate_iss();

	/*
		Returns a free port for outgoing connections.
	*/
	TCP::Port free_port();

	/*
		Packet is dropped.
	*/
	void drop(TCP::Packet_ptr);


}; // < class TCP

}; // < namespace net

#endif
