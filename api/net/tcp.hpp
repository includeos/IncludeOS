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

#include <os>
#include <net/ip4.hpp> // IP4::Addr
#include <net/ip4/packet_ip4.hpp> // PacketIP4
#include <net/util.hpp> // net::Packet_ptr, htons / noths
#include <vector>
#include <queue>
#include <map>
#include <sstream>

namespace net {

class TCP {
public:
	using Address = IP4::addr;
	using Port = uint16_t;
	/*
		A Sequence number (SYN/ACK) (32 bits)
	*/
	using Seq = uint32_t;
	class Packet;
	using Packet_ptr = std::shared_ptr<Packet>;
	using IPStack = Inet<LinkLayer,IP4>;

public:
	/*
		An IP address and a Port.
	*/
	class Socket {
	public:
		/*
			Intialize an empty socket.
		*/
		inline Socket() : address_(), port_(0) { address_.whole = 0; };

		/*
			Create a socket with a Address and Port.
		*/
		inline Socket(Address address, Port port) : address_(address), port_(port) {};

		/*
			Returns the Socket's address.
		*/
		inline const TCP::Address address() const { return address_; }

		/*
			Returns the Socket's port.
		*/
		inline TCP::Port port() const { return port_; }

		/*
			Returns a string in the format "Address:Port".
		*/
		std::string to_string() const {
			std::stringstream ss;
			ss << address_.str() << ":" << port_;
			return ss.str();
		}

		inline bool is_empty() const { return (address_.whole == 0 and port_ == 0); }

		/*
			Comparator used for vector.
		*/
		inline bool operator ==(const Socket &s2) const {
			return address().whole == s2.address().whole 
					and port() == s2.port();
		}

		/*
			Comparator used for map.
		*/
		inline bool operator <(const Socket& s2) const {
        	return address().whole < s2.address().whole
        		or (address().whole == s2.address().whole and port() < s2.port());
    	}

	private:
		//SocketID id_; // Maybe a hash or something. Not sure if needed (yet)
		TCP::Address address_;
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
		FIN 	= 1,			// Fin(ish)
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
		inline uint8_t offset() { return (uint8_t)(offset_flags.offset_reserved >> 4); }

		// Set raw TCP offset in quadruples
		inline void set_offset(uint8_t offset){ offset_flags.offset_reserved = (offset << 4); }

		// Get tcp header length including options (offset) in bytes
		inline uint8_t size() { return offset() * 4; }

		// Calculate the full header lenght, down to linklayer, in bytes
		uint8_t all_headers_len() { return (sizeof(TCP::Full_header) - sizeof(TCP::Header)) + size(); }

		inline void set_flag(Flag f){ offset_flags.whole |= htons(f); }
		inline void set_flags(uint16_t f){ offset_flags.whole |= htons(f); }
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
		TCP Checksum-header (TCP-header + pseudo-header)
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
    		set_payload(buffer() + sizeof(TCP::Full_header));
    	}

    	// GETTERS
    	inline TCP::Port src_port() const {
			return ntohs(header().source_port);
    	}

    	inline TCP::Port dst_port() const {
    		return ntohs(header().destination_port);
    	}

    	inline TCP::Seq seq() const {
    		return ntohl(header().seq_nr);
    	}

    	inline TCP::Seq ack() const {
    		return ntohl(header().ack_nr);
    	}

    	inline uint16_t win() const {
    		return ntohs(header().window_size);
    	}

    	inline TCP::Socket source() const { 
    		return TCP::Socket{src(), src_port()};
    	}

    	inline TCP::Socket destination() const {
    		return TCP::Socket{dst(), dst_port()};
    	}

    	// SETTERS
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

    	inline TCP::Packet& set_win_size(uint16_t size) { 
    		header().window_size = htons(size);
    		return *this;
    	}

    	inline TCP::Packet& set_checksum(uint16_t checksum) {
    		header().checksum = checksum;
    		return *this;
    	}

    	inline TCP::Packet& set_source(const TCP::Socket& src) {
    		set_src(src.address()); // PacketIP4::set_src
    		set_src_port(src.port());
    		return *this;
    	}

    	inline TCP::Packet& set_destination(const TCP::Socket& dest) {
    		set_dst(dest.address()); // PacketIP4::set_dst
    		set_dst_port(dest.port());
    		return *this;
    	}

    	/// FLAGS / CONTROL BITS ///

    	inline TCP::Packet& set_flag(TCP::Flag f) { 
    		header().set_flag(f);
    		return *this;
    	}

    	inline TCP::Packet& set_flags(uint16_t f) { 
    		header().set_flags(f);
    		return *this;
    	}

    	inline TCP::Packet& clear_flag(TCP::Flag f) {
    		header().clear_flag(f);
    		return *this;
    	}

    	inline TCP::Packet& clear_flags() {
    		header().clear_flags();
    		return *this;
    	}

    	inline bool isset(TCP::Flag f) { 
    		return ntohs(header().offset_flags.whole) & f; 
    	}

    	inline uint16_t data_length() const
    	{      
    		return size() - header().all_headers_len();
    	}

    	inline bool has_data() const {
    		return data_length() > 0;
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
    	/*uint16_t gen_checksum() { 
    		header().checksum = TCP::checksum(net::Packet::shared_from_this());
    		return header().checksum;
    	}*/

    	

	    //! assuming the packet has been properly initialized,
	    //! this will fill bytes from @buffer into this packets buffer,
	    //! then return the number of bytes written. buffer is unmodified
    	uint32_t fill(const char* buffer, size_t length)
    	{
    		uint32_t rem = capacity() - HEADERS_SIZE;
    		uint32_t total = (length < rem) ? length : rem;
      		// copy from buffer to packet buffer
    		memcpy(data() + data_length(), buffer, total);
    		//std::copy(send_buffer.begin(), send_buffer.begin()+total, buffer() + HEADERS_SIZE);
      		// set new packet length
    		set_length(data_length() + total);
    		return total;
    	}

    	inline std::string to_string() {
    		std::ostringstream os;
    		os << "[ S:" << source().to_string() << " D:" <<  destination().to_string()
    			<< " SEQ:" << seq() << " ACK:" << ack() << " DATA-LENGTH:" << data_length() 
    			<< " WIN:" << win() << " FLAGS:" << std::bitset<8>{header().offset_flags.flags}  << " ]";
    		return os.str();
    	}

	}; // << class TCP::Packet

	/*
		TODO: Implement.
	*/
	class TCPException : public std::runtime_error {
	public:
		TCPException(std::string error) : std::runtime_error(error) {};
	};


	/*
		A connection between two Sockets (local and remote).
		Receives and handle TCP::Packet.
		Transist between many states.
	*/
	class Connection {
	public:
		/*
			Connection identifier
		*/
		using Tuple = std::pair<TCP::Socket, TCP::Socket>;

		/*
			Callbacks
		*/
		using ConnectCallback 			= delegate<void(Connection&)>;
		using DataCallback 				= delegate<void(Connection&, bool)>;
		using DisconnectCallback		= delegate<void(Connection&, std::string)>;
		using ErrorCallback 			= delegate<void(Connection&, TCPException)>;
		using PacketReceivedCallback 	= delegate<void(Connection&, TCP::Packet_ptr)>;
		using PacketDroppedCallback		= delegate<void(TCP::Packet_ptr, std::string)>;

		/*
			Buffer
		*/
		using Buffer = std::queue<TCP::Packet_ptr>;

		/*
			Interface for one of the many states a Connection can have.
		*/
		class State {
		public:
			/*
				Open a Connection.
				OPEN
			*/
			virtual void open(Connection&, bool active = false);

			/*
				Write to a Connection.
				SEND
			*/
			virtual size_t send(Connection&, const char* buffer, size_t n, bool push = false);

			/*
				Read from a Connection.
				RECEIVE
			*/
			virtual size_t receive(Connection&, char* buffer, size_t n);
			
			/*
				Close a Connection.
				CLOSE
			*/
			virtual void close(Connection&);
			
			/*
				Handle a Packet
				SEGMENT ARRIVES
			*/
			virtual int handle(Connection&, TCP::Packet_ptr in);

			/*
				The current state represented as a string.
				STATUS
			*/
			virtual std::string to_string() const;

		protected:		
			/*
				Helper functions
				TODO: Clean up names.
			*/
			virtual bool check_seq(Connection&, TCP::Packet_ptr);

			virtual void unallowed_syn_reset_connection(Connection&, TCP::Packet_ptr);

			virtual bool check_ack(Connection&, TCP::Packet_ptr);

			virtual void process_segment(Connection&, TCP::Packet_ptr);

			virtual void process_fin(Connection&, TCP::Packet_ptr);

		}; // < class TCP::Connection::State

		/*
			Forward declaration of concrete states.
			Definition in "tcp_connection_states.hpp"
		*/
		class Closed;
		class Listen;
		class SynSent;
		class SynReceived;
		class Established;
		class CloseWait;
		class FinWait1;
		class FinWait2;
		class LastAck;
		class Closing;
		class TimeWait;

		/*
			Transmission Control Block.
			Keep tracks of all the data for a connection.
			
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

			TCB() {
				SND = { 0, 0, TCP::default_window_size, 0, 0, 0 };
				ISS = 0;
				RCV = { 0, TCP::default_window_size, 0 };
				IRS = 0;
			};

			std::string to_string() const;
		}__attribute__((packed)); // < struct TCP::Connection::TCB

		/*
			Creates a connection without a remote.
		*/
		Connection(TCP& host, Socket& local);

		/*
			Creates a connection with a remote.
		*/
		Connection(TCP& host, Socket& local, Socket remote);

		/*
			The hosting TCP instance.
		*/
		inline const TCP& host() const { return host_; }

		/*
			The local Socket bound to this connection.
		*/
		inline TCP::Socket local() const { return {host_.inet_.ip_addr(), local_port_}; }

		/*
			The remote Socket bound to this connection.
		*/
		inline TCP::Socket remote() const { return remote_; }

		/*
			Set remote Socket bound to this connection.
			@WARNING: Should this be public? Used by TCP.
		*/
		inline void set_remote(Socket remote) { remote_ = remote; }


		/*
			Read content from remote.
		*/
		size_t read(char* buffer, size_t n);

		/*
			Read n bytes into a string.
			Default 1024 bytes.
		*/
		std::string read(size_t n = 1024);

		/*
			Write content to remote.
		*/
		size_t write(const char* buffer, size_t n);

		/*
			Open connection.
		*/
		void open(bool active = false);

		/*
			Close connection.
		*/
		void close();

		/*
			Set callback for CONNECT event.
		*/
		inline Connection& onConnect(ConnectCallback callback) {
			on_connect_ = callback;
			return *this;
		}

		/*
			Set callback for ON DATA event.
		*/
		inline Connection& onData(DataCallback callback) {
			on_data_ = callback;
			return *this;
		}

		/*
			Set callback for DISCONNECT event.
		*/
		inline Connection& onDisconnect(DisconnectCallback callback) {
			on_disconnect_ = callback;
			return *this;
		}

		/*
			Set callback for ERROR event.
		*/
		inline Connection& onError(ErrorCallback callback) {
			on_error_ = callback;
			return *this;
		}

		/*
			Set callback for every packet received.
		*/
		inline Connection& onPacketReceived(PacketReceivedCallback callback) {
			on_packet_received_ = callback;
			return *this;
		}

		/*
			Set callback for when a packet is dropped.
		*/
		inline Connection& onPacketDropped(PacketDroppedCallback callback) {
			on_packet_dropped_ = callback;
			return *this;
		}

		/*
			Represent the Connection as a string (STATUS).
		*/
		std::string to_string() const;

		/*
			Returns the current state of the connection.
		*/
		inline Connection::State& state() const { return *state_; }

		/*
			Returns the previous state of the connection.
		*/
		inline Connection::State& prev_state() const { return *prev_state_; }

		/*
			Calculates and return bytes transmitted.
		*/
		inline uint32_t bytes_transmitted() const {
			return control_block.SND.NXT - control_block.ISS;
		}

		/*
			Calculates and return bytes received.
		*/
		inline uint32_t bytes_received() const {
			return control_block.RCV.NXT - control_block.IRS;
		}

		/*
			Return the id (TUPLE) of the connection.
		*/
		inline Connection::Tuple tuple() {
			return {local(), remote_};
		}

		/*
			Receive buffer
		*/
		inline Buffer& receive_buffer() {
			return receive_buffer_;
		}

		/*
			Send buffer
		*/
		inline Buffer& send_buffer() {
			return send_buffer_;
		}

		/*
			Receive a TCP Packet.

			@WARNING: Public, for use in TCP::bottom (friend it?)
		*/
		void receive(TCP::Packet_ptr);


		/*
			State checks.
		*/
		inline bool is_listening() const { return is_state("LISTENING"); }

		inline bool is_connected() const { return is_state("ESTABLISHED"); }

		inline bool is_closing() const { return (is_state("CLOSING") or is_state("LAST-ACK") or is_state("TIME-WAIT")); }

		/*
			Destroy the Connection.

			Clean up.
		*/
		~Connection();

	private:
		/* 
			"Parent" for Connection.
		*/
		TCP& host_;				// 4 B

		/* 
			End points.
		*/
		TCP::Port local_port_;	// 2 B
		TCP::Socket remote_;	// 8~ B

		/* 
			The current state the Connection is in. 
			Handles most of the logic. 
		*/
		State* state_;			// 4 B
		// Previous state. Used to keep track of state transitions.
		State* prev_state_;		// 4 B
		
		/* 
			Keep tracks of all sequence variables.
		*/
		TCB control_block;		// 36 B

		/* 
			Buffers
		*/
		Buffer receive_buffer_;
		Buffer send_buffer_;
		// How far in the most front packet is read.
		uint32_t rcv_buffer_offset = 0;

		
		/// CALLBACK HANDLING ///
		
		/* When Connection is ESTABLISHED. */
		ConnectCallback on_connect_ = [](Connection& conn) {
			printf("<TCP::Connection::@Connect> Connected.");
		};
		
		/* When Connection is CLOSING. */
		DisconnectCallback on_disconnect_ = [](Connection& conn, std::string msg) {
			printf("<TCP::Connection::@Disconnect> Connection disconnect. Reason: %s \n", msg.c_str());
		};
		
		/* When data is received */
		DataCallback on_data_ = [](Connection& conn, bool push) {
			printf("<TCP::Connection::@Data> Connection received data. Push: %d \n", push);
		};

		/* When error occcured. */
		ErrorCallback on_error_ = ErrorCallback::from<Connection,&Connection::error>(this);
		inline void error(Connection& conn, TCPException error) { 
			printf("<TCP::Connection::@Error> TCPException: %s \n", error.what()); 
		}

		/* When packet is received */
		PacketReceivedCallback on_packet_received_ = [](Connection& conn, TCP::Packet_ptr packet) {
			printf("<TCP::Connection::@PacketReceived> Packet received: %s \n", packet->to_string().c_str());
		};

		/* When a packet is dropped. */
		PacketDroppedCallback on_packet_dropped_ = [](TCP::Packet_ptr packet, std::string reason) { 
			printf("<TCP::Connection::@PacketDropped> Packet dropped. %s | Reason: %s \n", 
				packet->to_string().c_str(), reason.c_str()); 
		};

		/*
			Invoke/signal the diffrent TCP events.
		*/
		inline void signal_connect() { on_connect_(*this); }

		inline void signal_data(bool PUSH) { on_data_(*this, PUSH); }

		inline void signal_disconnect(std::string message) { on_disconnect_(*this, message); }

		inline void signal_error(TCPException error) { on_error_(*this, error); }

		inline void signal_packet_received(TCP::Packet_ptr packet) { on_packet_received_(*this, packet); }

		inline void signal_packet_dropped(TCP::Packet_ptr packet, std::string reason) { on_packet_dropped_(packet, reason); }

		/*
			Drop a packet. Used for debug/callback.
		*/
		inline void drop(TCP::Packet_ptr packet, std::string reason) { signal_packet_dropped(packet, reason); }
		inline void drop(TCP::Packet_ptr packet) { drop(packet, "None given."); }


		/// TCB HANDLING ///

		/*
			Returns the TCB.
		*/
		inline Connection::TCB& tcb() { return control_block; }

		/*
			Generate a new ISS.
		*/
		TCP::Seq generate_iss();


		/// STATE HANDLING ///
		/*
			Set state. (used by substates)
		*/
		inline void set_state(State& state) {
			prev_state_ = state_;
			state_ = &state;
			printf("<TCP::Connection::set_state> State changed for %p: %s => %s \n", 
					this, prev_state_->to_string().c_str(), state_->to_string().c_str());
		}

		/*
			Helper function for state checks.
		*/
		inline bool is_state(const std::string& state) const { 
			return state == state_->to_string();
		}


		/// BUFFER HANDLING ///

		/*
			Read from receive buffer.
		*/
		size_t read_from_receive_buffer(char* buffer, size_t n);

		/*
			Add(queue) a packet to the receive buffer.
		*/
		bool add_to_receive_buffer(TCP::Packet_ptr packet);
		
		/*
			Write to the send buffer. Segmentize into packets.
		*/
		size_t write_to_send_buffer(const char* buffer, size_t n);

		/*
			Transmit the send buffer.
		*/
		void transmit();

		/*
			Creates a new outgoing packet and put it in the back of the send buffer.
		*/
		TCP::Packet_ptr create_outgoing_packet();

		/*
		 	Returns the packet in the back of the send buffer.
		 	If the send buffer is empty, it creates a new packet and adds it.
		*/
	 	TCP::Packet_ptr outgoing_packet();		

	}; // < class TCP::Connection


	/// USER INTERFACE - TCP ///

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
	void bind(Port port, Connection::ConnectCallback);

	/*
		Active open a new connection to the given remote.
	*/
	Connection& connect(Socket remote);

	/*
		Active open a new connection to the given remote.
	*/
	void connect(Socket remote, Connection::ConnectCallback);

	/*
		Receive packet from network layer (IP).
	*/
	int bottom(net::Packet_ptr);

	/* 
		Delegate output to network layer
	*/
	inline void set_network_out(downstream del) { _network_layer_out = del; }

	/*
		Compute the TCP checksum
	*/
    static uint16_t checksum(const TCP::Packet_ptr);

    /*
		Number of open ports.
    */
	inline size_t openPorts() { return listeners.size(); }

	/*
		Show all connections for TCP as a string.
	*/
	std::string status() const;


private:
	IPStack& inet_;
	std::map<TCP::Port, Connection> listeners;
	std::map<Connection::Tuple, Connection> connections;

	downstream _network_layer_out;

	TCP::Port current_ephemeral_ = 1024;
	
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

	/*
		Add a Connection.
	*/
	TCP::Connection& add_connection(TCP::Socket& local, TCP::Socket remote);


}; // < class TCP

}; // < namespace net

#endif
