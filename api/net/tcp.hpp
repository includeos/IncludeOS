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
#include <queue> // buffer
#include <map>
#include <sstream> // ostringstream
#include <chrono> // timer duration
#include <memory> // enable_shared_from_this

inline unsigned round_up(unsigned n, unsigned div) {
	assert(n);
	return (n + div - 1) / div;
}

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

	class TCPException;
	class TCPBadOptionException;

	class Connection;
	using Connection_ptr = std::shared_ptr<Connection>;
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

	static constexpr uint16_t default_mss = 536;

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
      	uint8_t options[0];			// Options
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
		TCP Header Option
	*/
	struct Option {
		uint8_t kind;
		uint8_t length;
		uint8_t data[0];

		enum Kind {
			END = 0x00, // End of option list
			NOP = 0x01, // No-Opeartion
			MSS = 0x02, // Maximum Segment Size [RFC 793] Rev: [879, 6691]
		};

		static std::string kind_string(Kind kind) {
			switch(kind) {
				case MSS:
					return {"MSS"};

				default:
					return {"Unknown Option"};
			}
		}

		struct opt_mss {
			uint8_t kind;
			uint8_t length;
			uint16_t mss;

			opt_mss(uint16_t mss)
				: kind(MSS), length(4), mss(htons(mss)) {}
		};

		struct opt_timestamp {
			uint8_t kind;
			uint8_t length;
			uint32_t ts_val;
			uint32_t ts_ecr;
		};
	};


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
    		memset(buffer(), 0, HEADERS_SIZE);
    		PacketIP4::init();

    		set_protocol(IP4::IP4_TCP);
    		set_win_size(TCP::default_window_size);
    		set_offset(5);

	      	// set TCP payload location (!?)
    		set_payload(buffer() + all_headers_len());
    	}

    	// GETTERS
    	inline TCP::Port src_port() const { return ntohs(header().source_port); }

    	inline TCP::Port dst_port() const { return ntohs(header().destination_port); }

    	inline TCP::Seq seq() const { return ntohl(header().seq_nr); }

    	inline TCP::Seq ack() const { return ntohl(header().ack_nr); }

    	inline uint16_t win() const { return ntohs(header().window_size); }

    	inline TCP::Socket source() const { return TCP::Socket{src(), src_port()}; }

    	inline TCP::Socket destination() const { return TCP::Socket{dst(), dst_port()}; }

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
    		header().offset_flags.whole |= htons(f);
    		return *this;
    	}

    	inline TCP::Packet& set_flags(uint16_t f) {
    		header().offset_flags.whole |= htons(f);
    		return *this;
    	}

    	inline TCP::Packet& clear_flag(TCP::Flag f) {
    		header().offset_flags.whole &= ~ htons(f);
    		return *this;
    	}

    	inline TCP::Packet& clear_flags() {
    		header().offset_flags.whole &= 0x00ff;
    		return *this;
    	}

    	inline bool isset(TCP::Flag f) { return ntohs(header().offset_flags.whole) & f; }


    	/// OFFSET, OPTIONS, DATA ///

    	// Get the raw tcp offset, in quadruples
		inline uint8_t offset() const { return (uint8_t)(header().offset_flags.offset_reserved >> 4); }

		// Set raw TCP offset in quadruples
		inline void set_offset(uint8_t offset) { header().offset_flags.offset_reserved = (offset << 4); }

		// The actaul TCP header size (including options).
		inline uint8_t header_size() const { return offset() * 4; }

		// Calculate the full header length, down to linklayer, in bytes
		uint8_t all_headers_len() const { return (HEADERS_SIZE - sizeof(TCP::Header)) + header_size(); }

		// Where data starts
		inline char* data() { return (char*) (buffer() + all_headers_len()); }

    	inline uint16_t data_length() const { return size() - all_headers_len(); }

    	inline bool has_data() const { return data_length() > 0; }

    	inline uint16_t tcp_length() const { return header_size() + data_length(); }

    	template <typename T, typename... Args>
    	inline void add_option(Args&&... args) {
    		// to avoid headache, options need to be added BEFORE any data.
    		assert(!has_data());
    		// option address
    		auto* addr = options()+options_length();
    		new (addr) T(args...);
    		// update offset
    		set_offset(offset() + round_up( ((T*)addr)->length, 4 ));
    		set_length(); // update
    	}

    	inline void clear_options() {
    		// clear existing options
    		// move data (if any) (??)
    		set_offset(5);
    		set_length(); // update
    	}

    	inline uint8_t* options() { return (uint8_t*) header().options; }

    	inline uint8_t options_length() const { return header_size() - sizeof(TCP::Header); }

    	inline bool has_options() const { return options_length() > 0; }

    	// sets the correct length for all the protocols up to IP4
    	void set_length(uint16_t newlen = 0) {
     		// new total packet length
    		set_size( all_headers_len() + newlen );
    	}

	    //! assuming the packet has been properly initialized,
	    //! this will fill bytes from @buffer into this packets buffer,
	    //! then return the number of bytes written. buffer is unmodified
    	size_t fill(const char* buffer, size_t length) {
    		size_t rem = capacity() - all_headers_len();
    		size_t total = (length < rem) ? length : rem;
      		// copy from buffer to packet buffer
    		memcpy(data() + data_length(), buffer, total);
      		// set new packet length
    		set_length(data_length() + total);
    		return total;
    	}

    	inline std::string to_string() {
    		std::ostringstream os;
    		os << "[ S:" << source().to_string() << " D:" <<  destination().to_string()
    			<< " SEQ:" << seq() << " ACK:" << ack()
    			<< " HEAD-LEN:" << (int)header_size() << " OPT-LEN:" << (int)options_length() << " DATA-LEN:" << data_length()
    			<< " WIN:" << win() << " FLAGS:" << std::bitset<8>{header().offset_flags.flags}  << " ]";
    		return os.str();
    	}

	}; // << class TCP::Packet

	/*
		TODO: Does this need to be better? (faster? stronger?)
	*/
	class TCPException : public std::runtime_error {
	public:
		TCPException(const std::string& error) : std::runtime_error(error) {};
		virtual ~TCPException() {};
	};

	/*
		Exception for Bad TCP Header Option (TCP::Option)
	*/
	class TCPBadOptionException : public TCPException {
	public:
		TCPBadOptionException(Option::Kind kind, const std::string& error) :
			TCPException("Bad Option [" + Option::kind_string(kind) + "]: " + error),
			kind_(kind) {};

		Option::Kind kind();
	private:
		Option::Kind kind_;
	};

	/*
		Buffer for TCP::Packet_ptr
	*/
	template<typename T = TCP::Packet_ptr, typename Buffer = std::queue<T>>
	class PacketBuffer {
	public:

		PacketBuffer(typename Buffer::size_type limit) :
			buffer_(), data_length_(0),
			data_offset_(0), limit_(limit)
		{

		}
		/* Number of packets */
		inline auto size() const { return buffer_.size(); }

		/* Amount of data */
		inline size_t data_size() const { return data_length_; }

		inline void push(const T& packet) {
			buffer_.push(packet);
			data_length_ += (size_t)packet->data_length();
		}

		inline bool add(T packet) {
			if(full()) return false;
			push(packet);
			return true;
		}

		inline void pop() {
			data_length_ -= (size_t)buffer_.front()->data_length();
			buffer_.pop();
		}

		inline const T& front() const { return buffer_.front(); }

		inline const T& back() const { return buffer_.back(); }

		inline bool empty() const { return buffer_.empty(); }

		inline auto limit() const { return limit_; }

		inline bool full() const { return size() >= limit(); }

		inline auto data_offset() const { return data_offset_; }

		inline void set_data_offset(uint32_t offset) { data_offset_ = offset; }

		inline void clear() {
			while(!buffer_.empty())
				buffer_.pop();
			data_length_ = {0};
			data_offset_ = {0};
		}

	private:
		Buffer buffer_;
		size_t data_length_;
		uint32_t data_offset_;
		typename Buffer::size_type limit_;

	}; // << TCP::PacketBuffer


	/*
		A connection between two Sockets (local and remote).
		Receives and handle TCP::Packet.
		Transist between many states.
	*/
	class Connection : public std::enable_shared_from_this<Connection> {
	public:
		/*
			Connection identifier
		*/
		using Tuple = std::pair<TCP::Port, TCP::Socket>;

		/// CALLBACKS ///
		/*
			On connection attempt - When a remote sends SYN to connection in LISTENING state.
			First thing that will happen.
		*/
		using AcceptCallback			= delegate<bool(std::shared_ptr<Connection>)>;

		/*
			On connected - When both hosts exchanged sequence numbers (handshake is done).
			Now in ESTABLISHED state - it's allowed to write and read to/from the remote.
		*/
		using ConnectCallback 			= delegate<void(std::shared_ptr<Connection>)>;

		/*
			On receiving data - When there is data to read in the receive buffer.
			Either when remote PUSH, or buffer is full.
		*/
		using ReceiveCallback 			= delegate<void(std::shared_ptr<Connection>, bool)>;

		/*
			On disconnect - When a remote told it wanna close the connection.
			Connection has received a FIN, currently last thing that will happen before a connection is remoed.
		*/
		struct Disconnect;

		using DisconnectCallback		= delegate<void(std::shared_ptr<Connection>, Disconnect)>;

		/*
			On error - When any of the users request fails.
		*/
		using ErrorCallback 			= delegate<void(std::shared_ptr<Connection>, TCPException)>;

		/*
			When a packet is received - Everytime a connection receives an incoming packet.
			Would probably be used for debugging.
			(Currently not in use)
		*/
		using PacketReceivedCallback 	= delegate<void(std::shared_ptr<Connection>, TCP::Packet_ptr)>;

		/*
			When a packet is dropped - Everytime an incoming packet is unallowed, it will be dropped.
			Can be used for debugging.
		*/
		using PacketDroppedCallback		= delegate<void(TCP::Packet_ptr, std::string)>;

		/*
			Buffer
		*/
		using Buffer = PacketBuffer<>;

		/*
			Reason for disconnect event.
		*/
		struct Disconnect {
		public:
			enum Reason {
				CLOSING,
				REFUSED,
				RESET
			};

			Reason reason;

			explicit Disconnect(Reason reason) : reason(reason) {}

			inline operator Reason() const noexcept { return reason; }

			inline operator std::string() const noexcept { return to_string(); }

			inline bool operator ==(const Disconnect& dc) const { return reason == dc.reason; }

			std::string to_string() const {
				switch(reason) {
				case CLOSING:
					return "Connection closing";
				case REFUSED:
					return "Conneciton refused";
				case RESET:
					return "Connection reset";
				default:
					return "Unknown reason";
				}
			}
		}; // < struct TCP::Connection::Disconnect

		/*
			Interface for one of the many states a Connection can have.
		*/
		class State {
		public:
			enum Result {
				CLOSED = -1,
				OK = 0,
				CLOSE = 1
			};
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
				Terminate a Connection.
				ABORT
			*/
			virtual void abort(Connection&);

			/*
				Handle a Packet
				SEGMENT ARRIVES
			*/
			virtual Result handle(Connection&, TCP::Packet_ptr in) = 0;

			/*
				The current state represented as a string.
				STATUS
			*/
			virtual std::string to_string() const = 0;

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

			virtual void send_reset(Connection&);

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
		class FinWait1;
		class FinWait2;
		class CloseWait;
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

				uint16_t MSS;	// Maximum segment size for outgoing segments.
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
				SND = { 0, 0, TCP::default_window_size, 0, 0, 0, TCP::default_mss };
				ISS = 0;
				RCV = { 0, TCP::default_window_size, 0 };
				IRS = 0;
			};

			std::string to_string() const;
		}__attribute__((packed)); // < struct TCP::Connection::TCB

		/*
			Creates a connection without a remote.
		*/
		Connection(TCP& host, Port local_port);

		/*
			Creates a connection with a remote.
		*/
		Connection(TCP& host, Port local_port, Socket remote);

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
		std::string read(size_t n = 0);

		/*
			Write content to remote.
		*/
		size_t write(const char* buffer, size_t n, bool PUSH = true);

		/*
			Write a string to the remote.
		*/
		inline void write(const std::string& content) {
			write(content.data(), content.size(), true);
		}

		/*
			Open connection.
		*/
		void open(bool active = false);

		/*
			Close connection.
		*/
		void close();

		/*
			Abort connection. (Same as Terminate)
		*/
		inline void abort() {
			state_->abort(*this);
			signal_close();
		}

		/*
			Set callback for ACCEPT event.
		*/
		inline Connection& onAccept(AcceptCallback callback) {
			on_accept_ = callback;
			return *this;
		}

		/*
			Set callback for CONNECT event.
		*/
		inline Connection& onConnect(ConnectCallback callback) {
			on_connect_ = callback;
			return *this;
		}

		/*
			Set callback for ON RECEIVE event.
		*/
		inline Connection& onReceive(ReceiveCallback callback) {
			on_receive_ = callback;
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
			TODO: Not sure if this will suffice.
		*/
		inline uint32_t bytes_transmitted() const {
			return control_block.SND.NXT - control_block.ISS;
		}

		/*
			Calculates and return bytes received.
			TODO: Not sure if this will suffice.
		*/
		inline uint32_t bytes_received() const {
			return control_block.RCV.NXT - control_block.IRS;
		}

		/*
			Return the id (TUPLE) of the connection.
		*/
		inline Connection::Tuple tuple() const {
			return {local_port_, remote_};
		}

		/*
			Receive buffer
		*/
		inline const Buffer& receive_buffer() {
			return receive_buffer_;
		}

		/*
			Send buffer
		*/
		inline const Buffer& send_buffer() {
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
		bool is_listening() const;

		bool is_connected() const;

		bool is_closing() const;

		bool is_writable() const;

		/*
			Helper function for state checks.
		*/
		inline bool is_state(const State& state) const {
			return state_ == &state;
		}

		inline bool is_state(const std::string& state_str) const {
			return state_->to_string() == state_str;
		}

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

		/*
			When time-wait timer was started.
		*/
		uint64_t time_wait_started;


		/// CALLBACK HANDLING ///

		/* When a Connection is initiated. */
		AcceptCallback on_accept_ = AcceptCallback::from<Connection,&Connection::default_on_accept>(this);
		inline bool default_on_accept(std::shared_ptr<Connection>) {
			//debug2("<TCP::Connection::@Accept> Connection attempt from: %s \n", conn->remote().to_string().c_str());
			return true; // Always accept
		}

		/* When Connection is ESTABLISHED. */
		ConnectCallback on_connect_ = [](std::shared_ptr<Connection>) {
			debug2("<TCP::Connection::@Connect> Connected.\n");
		};

		/* When data is received */
		ReceiveCallback on_receive_ = [](std::shared_ptr<Connection>, bool) {
			debug2("<TCP::Connection::@Receive> Connection received data. \n");
		};

		/* When Connection is CLOSING. */
		DisconnectCallback on_disconnect_ = [](std::shared_ptr<Connection>, Disconnect) {
			//debug2("<TCP::Connection::@Disconnect> Connection disconnect. Reason: %s \n", msg.c_str());
		};

		/* When error occcured. */
		ErrorCallback on_error_ = ErrorCallback::from<Connection,&Connection::default_on_error>(this);
		inline void default_on_error(std::shared_ptr<Connection>, TCPException) {
			//debug2("<TCP::Connection::@Error> TCPException: %s \n", error.what());
		}

		/* When packet is received */
		PacketReceivedCallback on_packet_received_ = [](std::shared_ptr<Connection>, TCP::Packet_ptr) {
			//debug2("<TCP::Connection::@PacketReceived> Packet received: %s \n", packet->to_string().c_str());
		};

		/* When a packet is dropped. */
		PacketDroppedCallback on_packet_dropped_ = [](TCP::Packet_ptr, std::string) {
			//debug("<TCP::Connection::@PacketDropped> Packet dropped. %s | Reason: %s \n",
			//	packet->to_string().c_str(), reason.c_str());
		};

		/*
			Invoke/signal the diffrent TCP events.
		*/
		inline bool signal_accept() { return on_accept_(shared_from_this()); }

		inline void signal_connect() { on_connect_(shared_from_this()); }

		inline void signal_receive(bool PUSH) { on_receive_(shared_from_this(), PUSH); }

		inline void signal_disconnect(Disconnect::Reason&& reason) { on_disconnect_(shared_from_this(), Disconnect{reason}); }

		inline void signal_error(TCPException error) { on_error_(shared_from_this(), error); }

		inline void signal_packet_received(TCP::Packet_ptr packet) { on_packet_received_(shared_from_this(), packet); }

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
		void set_state(State& state);


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
		size_t write_to_send_buffer(const char* buffer, size_t n, bool PUSH = true);

		/*
			Transmit the send buffer.
		*/
		void transmit();

		/*
			Transmit the packet.
		*/
		void transmit(TCP::Packet_ptr);

		/*
			Creates a new outgoing packet and put it in the back of the send buffer.
		*/
		TCP::Packet_ptr create_outgoing_packet();

		/*
		 	Returns the packet in the back of the send buffer.
		 	If the send buffer is empty, it creates a new packet and adds it.
		*/
	 	TCP::Packet_ptr outgoing_packet();


	 	/// RETRANSMISSION ///

	 	/*
	 		Starts a retransmission timer that retransmits the packet when RTO has passed.

	 		// TODO: Calculate RTO, currently hardcoded to 1 second (1000ms).
	 	*/
	 	void add_retransmission(TCP::Packet_ptr);

	 	/*
			Measure the elapsed time between sending a data octet with a
      		particular sequence number and receiving an acknowledgment that
      		covers that sequence number (segments sent do not have to match
      		segments received).  This measured elapsed time is the Round Trip
      		Time (RTT).
	 	*/
	 	//std::chrono::milliseconds RTT() const;
  		std::chrono::milliseconds RTO() const;

  		/*
			Start the time wait timeout for 2*MSL
  		*/
	 	void start_time_wait_timeout();

	 	/*
			Tell the host (TCP) to delete this connection.
	 	*/
	 	void signal_close();


	 	/// OPTIONS ///
	 	/*
			Maximum Segment Data Size
			(Limit the size for outgoing packets)
	 	*/
	 	inline uint16_t MSDS() const {
	 		return std::min(host_.MSS(), control_block.SND.MSS);
	 	}

	 	/*
			Parse and apply options.
	 	*/
	 	void parse_options(TCP::Packet_ptr);

	 	/*
			Add an option.
	 	*/
		void add_option(TCP::Option::Kind, TCP::Packet_ptr);

	}; // < class TCP::Connection


	/// USER INTERFACE - TCP ///

	/*
		Constructor
	*/
	TCP(IPStack&);

	/*
		Bind a new listener to a given Port.
	*/
	TCP::Connection& bind(Port port);

	/*
		Active open a new connection to the given remote.
	*/
	Connection_ptr connect(Socket remote);

	/*
		Active open a new connection to the given remote.
	*/
	inline auto connect(TCP::Address address, Port port = 80) {
		return connect({address, port});
	}

	/*
		Active open a new connection to the given remote.
	*/
	void connect(Socket remote, Connection::ConnectCallback);

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

    inline const auto& listeners() { return listeners_; }

    inline const auto& connections() { return connections_; }

    /*
		Number of open ports.
    */
	inline size_t openPorts() { return listeners_.size(); }

	/*
		Number of active connections.
	*/
	inline size_t activeConnections() { return connections_.size(); }

	/*
		Maximum Segment Lifetime
	*/
	inline auto MSL() const { return MAX_SEG_LIFETIME; }

	/*
		Set Maximum Segment Lifetime
	*/
	inline void set_MSL(const std::chrono::milliseconds msl) {
		MAX_SEG_LIFETIME = msl;
	}

	/*
		Maximum Buffer Size
	*/
	inline auto buffer_limit() const { return MAX_BUFFER_SIZE; }

	/*
		Set Buffer limit
	*/
	inline void set_buffer_limit(size_t buffer_limit) {
		MAX_BUFFER_SIZE = buffer_limit;
	}

	/*
		Maximum Segment Size
		[RFC 793] [RFC 879] [RFC 6691]

		@NOTE: Currently not supporting MTU bigger than 1482 bytes.
	*/
	inline uint16_t MSS() const {
		/*
			VirtulaBox "issue":
			MTU > 1498 will break TCP.
			MTU > 1482 seems to cause fragmentation: https://www.virtualbox.org/ticket/13967
		*/
		const uint16_t VBOX_LIMIT = 1482;
		return std::min(inet_.MTU(), VBOX_LIMIT) - sizeof(Full_header::ip4) - sizeof(TCP::Header);
	}

	/*
		Show all connections for TCP as a string.
	*/
	std::string status() const;


private:
	IPStack& inet_;
	std::map<TCP::Port, Connection> listeners_;
	std::map<Connection::Tuple, Connection_ptr> connections_;

	downstream _network_layer_out;

	/*
		Settings
	*/
	TCP::Port current_ephemeral_ = 1024;

	std::chrono::milliseconds MAX_SEG_LIFETIME;

	/*
		Current: limit by packet COUNT.
		Connection buffer size in bytes = buffers * BUFFER_LIMIT * MTU.
	*/
	size_t MAX_BUFFER_SIZE = 10;

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
	Connection_ptr add_connection(TCP::Port local_port, TCP::Socket remote);

	/*
		Close and delete the connection.
	*/
	void close_connection(TCP::Connection&);


}; // < class TCP

}; // < namespace net

#endif
