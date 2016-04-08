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
#include "ip4/ip4.hpp" // IP4::Addr
#include "ip4/packet_ip4.hpp" // PacketIP4
#include "util.hpp" // net::Packet_ptr, htons / noths
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

    using buffer_t = std::shared_ptr<uint8_t>;

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
      NS                = (1 << 8),     // Nounce (Experimental: see RFC 3540)
      CWR       = (1 << 7),             // Congestion Window Reduced
      ECE       = (1 << 6),             // ECN-Echo
      URG       = (1 << 5),             // Urgent
      ACK       = (1 << 4),             // Acknowledgement
      PSH       = (1 << 3),             // Push
      RST       = (1 << 2),             // Reset
      SYN       = (1 << 1),             // Syn(chronize)
      FIN       = 1,                    // Fin(ish)
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
      TCP::Port source_port;                    // Source port
      TCP::Port destination_port;               // Destination port
      uint32_t seq_nr;                          // Sequence number
      uint32_t ack_nr;                          // Acknowledge number
      union {
        uint16_t whole;                         // Reference to offset_reserved & flags together.
        struct {
          uint8_t offset_reserved;      // Offset (4 bits) + Reserved (3 bits) + NS (1 bit)
          uint8_t flags;                                // All Flags (Control bits) except NS (9 bits - 1 bit)
        };
      } offset_flags;                                   // Data offset + Reserved + Flags (16 bits)
      uint16_t window_size;                     // Window size
      uint16_t checksum;                                // Checksum
      uint16_t urgent;                          // Urgent pointer offset
      uint8_t options[0];                       // Options
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
        set_win(TCP::default_window_size);
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

      inline TCP::Packet& set_win(uint16_t size) {
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
      A connection between two Sockets (local and remote).
      Receives and handle TCP::Packet.
      Transist between many states.
    */
    class Connection : public std::enable_shared_from_this<Connection> {
      friend class TCP;
    public:

      /*
        Wrapper around a buffer that receives data.
      */
      struct ReadBuffer {
        buffer_t buffer;
        size_t remaining;
        size_t offset;
        bool push;

        ReadBuffer(buffer_t buf, size_t length, size_t offs = 0)
          : buffer(buf), remaining(length-offs), offset(offs), push(false) {}

        inline size_t capacity() const { return remaining + offset; }

        inline bool empty() const { return offset == 0; }

        inline bool full() const { return remaining == 0; }

        inline size_t size() const { return offset; }

        inline uint8_t* begin() const { return buffer.get(); }

        inline uint8_t* pos() const { return buffer.get() + offset; }

        inline uint8_t* end() const { return buffer.get() + capacity(); }

        inline bool advance(size_t length) {
          assert(length <= remaining);
          offset += length;
          remaining -= length;
          return length > 0;
        }

        inline size_t add(uint8_t* data, size_t n) {
          auto written = std::min(n, remaining);
          memcpy(pos(), data, written);
          return written;
        }

        inline void clear() {
          memset(begin(), 0, offset);
          remaining = capacity();
          offset = 0;
          push = false;
        }
      }; // < Connection::ReadBuffer


      /*
        Wrapper around a buffer that contains data to be written.
      */
      struct WriteBuffer {
        buffer_t buffer;
        size_t remaining;
        size_t offset;
        bool push;

        WriteBuffer(buffer_t buf, size_t length, bool PSH, size_t offs = 0)
          : buffer(buf), remaining(length-offs), offset(offs), push(PSH) {}

        inline size_t length() const { return remaining + offset; }

        inline bool done() const { return remaining == 0; }

        inline uint8_t* begin() const { return buffer.get(); }

        inline uint8_t* pos() const { return buffer.get() + offset; }

        inline uint8_t* end() const { return buffer.get() + length(); }

        inline bool advance(size_t length) {
          assert(length <= remaining);
          offset += length;
          remaining -= length;
          return length > 0;
        }
      }; // < Connection::WriteBuffer

      /*
        Callback when a receive buffer receives either push or is full
        - Supplied on asynchronous read
      */
      using ReadCallback = delegate<void(buffer_t, size_t)>;

      struct ReadRequest {
        ReadBuffer buffer;
        ReadCallback callback;

        ReadRequest(ReadBuffer buf, ReadCallback cb) : buffer(buf), callback(cb) {}
        ReadRequest(size_t n = 0) :
          buffer(buffer_t(new uint8_t[n], std::default_delete<uint8_t[]>()), n),
          callback([](auto, auto){}) {}
      };

      /*
        Callback when a write is sent by the TCP
        - Supplied on asynchronous write
      */
      using WriteCallback = delegate<void(size_t)>;

      using WriteRequest = std::pair<WriteBuffer, WriteCallback>;

      /*
        Connection identifier
      */
      using Tuple = std::pair<TCP::Port, TCP::Socket>;

      /// CALLBACKS ///
      /*
        On connection attempt - When a remote sends SYN to connection in LISTENING state.
        First thing that will happen.
      */
      using AcceptCallback                      = delegate<bool(std::shared_ptr<Connection>)>;

      /*
        On connected - When both hosts exchanged sequence numbers (handshake is done).
        Now in ESTABLISHED state - it's allowed to write and read to/from the remote.
      */
      using ConnectCallback                     = delegate<void(std::shared_ptr<Connection>)>;

      /*
        On disconnect - When a remote told it wanna close the connection.
        Connection has received a FIN, currently last thing that will happen before a connection is remoed.
      */
      struct Disconnect;

      using DisconnectCallback          = delegate<void(std::shared_ptr<Connection>, Disconnect)>;

      /*
        On error - When any of the users request fails.
      */
      using ErrorCallback                         = delegate<void(std::shared_ptr<Connection>, TCPException)>;

      /*
        When a packet is received - Everytime a connection receives an incoming packet.
        Would probably be used for debugging.
        (Currently not in use)
      */
      using PacketReceivedCallback      = delegate<void(std::shared_ptr<Connection>, TCP::Packet_ptr)>;

      /*
        When a packet is dropped - Everytime an incoming packet is unallowed, it will be dropped.
        Can be used for debugging.
      */
      using PacketDroppedCallback               = delegate<void(TCP::Packet_ptr, std::string)>;


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
        virtual size_t send(Connection&, WriteBuffer&);

        /*
          Read from a Connection.
          RECEIVE
        */
        virtual void receive(Connection&, ReadBuffer&);

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

        /*

         */
        virtual bool is_connected() const { return false; }

        virtual bool is_writable() const { return false; }

        virtual bool is_readable() const { return false; }

        virtual bool is_closing() const { return false; }

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
      class Closing;
      class LastAck;
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
          TCP::Seq UNA; // send unacknowledged
          TCP::Seq NXT; // send next
          uint16_t WND; // send window
          uint16_t UP;  // send urgent pointer
          TCP::Seq WL1; // segment sequence number used for last window update
          TCP::Seq WL2; // segment acknowledgment number used for last window update

          uint16_t MSS; // Maximum segment size for outgoing segments.

          uint32_t cwnd; // Congestion window [RFC 5681]
        } SND; // <<
        TCP::Seq ISS;           // initial send sequence number

        /* Receive Sequence Variables */
        struct {
          TCP::Seq NXT; // receive next
          uint16_t WND; // receive window
          uint16_t UP;  // receive urgent pointer

          uint16_t rwnd; // receivers advertised window [RFC 5681]
        } RCV; // <<
        TCP::Seq IRS;           // initial receive sequence number

        uint32_t ssthresh; // slow start threshold [RFC 5681]

        TCB() {
          SND = { 0, 0, TCP::default_window_size, 0, 0, 0, TCP::default_mss, 0 };
          ISS = 0;
          RCV = { 0, TCP::default_window_size, 0, 0 };
          IRS = 0;
          ssthresh = TCP::default_window_size;
        };

        bool slow_start() const {
          return SND.cwnd <= ssthresh;
        }

        bool congestion_avoidance() const {
          return !slow_start();
        }

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
        Read asynchronous from a remote.

        Create n sized internal read buffer and callback for when data is received.
        Callback will be called until overwritten with a new read() or connection closes.
        Buffer is cleared for data after every reset.
      */
      inline void read(size_t n, ReadCallback callback) {
        ReadBuffer buffer = {buffer_t(new uint8_t[n], std::default_delete<uint8_t[]>()), n};
        read(buffer, callback);
      }

      /*
        Assign the connections receive buffer and callback for when data is received.
        Works as read(size_t, ReadCallback);
      */
      inline void read(buffer_t buffer, size_t n, ReadCallback callback) {
        read({buffer, n}, callback);
      }

      void read(ReadBuffer buffer, ReadCallback callback);


      /*
        Write asynchronous to a remote.

        Copies the data from the buffer into an internal buffer. Callback is called when a a write is either done or aborted.
        Immediately tries to write the data to the connection. If not possible, queues the write for processing when possible (FIFO).
      */
      inline void write(const void* buf, size_t n, WriteCallback callback, bool PUSH = true) {
        auto buffer = buffer_t(new uint8_t[n], std::default_delete<uint8_t[]>());
        memcpy(buffer.get(), buf, n);
        write(buffer, n, callback, PUSH);
      }

      /*
        Works as write(const void*, size_t, WriteCallback, bool),
        but with the exception of avoiding copying the data to an internal buffer.
      */
      inline void write(buffer_t buffer, size_t n, WriteCallback callback, bool PUSH = true) {
        write({buffer, n, PUSH}, callback);
      }

      /*
        Works the same as it's counterpart, without subscribing to a WriteCallback.
      */
      inline void write(const void* buf, size_t n, bool PUSH = true) {
        write(buf, n, [](auto){}, PUSH);
      }

      /*
        Works the same as it's counterpart, without subscribing to a WriteCallback.
      */
      inline void write(buffer_t buffer, size_t n, bool PUSH = true) {
        write({buffer, n, PUSH}, [](auto){});
      }

      void write(WriteBuffer request, WriteCallback callback);


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
        Bytes queued for transmission.
        TODO: Implement when retransmission is up and running.
      */
      //inline size_t send_queue_bytes() const {}

      /*
        Bytes currently in receive buffer.
      */
      inline size_t read_queue_bytes() const {
        return read_request.buffer.size();
      }

      /*
        Return the id (TUPLE) of the connection.
      */
      inline Connection::Tuple tuple() const {
        return {local_port_, remote_};
      }


      /*
        State checks.
      */
      bool is_listening() const;

      inline bool is_connected() const { return state_->is_connected(); }

      inline bool is_writable() const { return state_->is_writable(); }

      inline bool is_readable() const { return state_->is_readable(); }

      inline bool is_closing() const { return state_->is_closing(); }



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
      TCP& host_;                               // 4 B

      /*
        End points.
      */
      TCP::Port local_port_;    // 2 B
      TCP::Socket remote_;      // 8~ B

      /*
        The current state the Connection is in.
        Handles most of the logic.
      */
      State* state_;                    // 4 B
      // Previous state. Used to keep track of state transitions.
      State* prev_state_;               // 4 B

      /*
        Keep tracks of all sequence variables.
      */
      TCB control_block;                // 36 B

      /*
        The given read request
      */
      ReadRequest read_request;

      /*
        Queue for write requests to process
      */
      std::queue<WriteRequest> write_queue;

      /*
        State if connection is in TCP write queue or not.
      */
      bool queued_;

      struct {
        TCP::Seq ACK = 0;
        size_t count = 0;
      } dup_acks_;

      /*
        Bytes queued for transmission.
      */
      //size_t write_queue_total;

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
        //      packet->to_string().c_str(), reason.c_str());
      };


      /// READING ///

      /*
        Assign the read request (read buffer)
      */
      inline void receive(ReadBuffer& buffer) {
        read_request.buffer = {buffer};
      }

      /*
        Receive data into the current read requests buffer.
      */
      size_t receive(const uint8_t* data, size_t n, bool PUSH);

      /*
        Copy data into the ReadBuffer
      */
      inline size_t receive(ReadBuffer& buf, const uint8_t* data, size_t n) {
        auto received = std::min(n, buf.remaining);
        memcpy(buf.pos(), data, received); // Can we use move?
        return received;
      }

      /*
        Remote is closing, no more data will be received.
        Returns receive buffer to user.
      */
      inline void receive_disconnect() {
        assert(!read_request.buffer.empty());
        auto& buf = read_request.buffer;
        buf.push = true;
        read_request.callback(buf.buffer, buf.size());
      }


      /// WRITING ///

      /*
        Active try to send a buffer by asking the TCP.
      */
      inline size_t send(WriteBuffer& buffer) {
        return host_.send(shared_from_this(), buffer);
      }

      /*
        Segmentize buffer into packets until either everything has been written,
        or all packets are used up.
      */
      size_t send(const char* buffer, size_t remaining, size_t& packets, bool PUSH);

      inline size_t send(WriteBuffer& buffer, size_t& packets) {
        return send((char*)buffer.pos(), buffer.remaining, packets, buffer.push);
      }

      /*
        Process the write queue with the given amount of packets.
        Returns true if the Connection finishes - there is no more doable jobs.
      */
      bool offer(size_t& packets);

      /*
        Returns if the connection has a doable write job.
      */
      inline bool has_doable_job() {
        return !write_queue.empty() and usable_window() >= MSDS();
      }

      /*
        Try to process the current write queue.
      */
      void write_queue_push();

      /*
        Try to write (some of) queue on connected.
      */
      inline void write_queue_on_connect() { write_queue_push(); }

      /*
        Reset queue on disconnect. Clears the queue and notice every requests callback.
      */
      void write_queue_reset();

      /*
        Returns if the TCP has the Connection in write queue
      */
      inline bool is_queued() const {
        return queued_;
      }

      /*
        Mark wether the Connection is in TCP write queue or not.
      */
      inline void set_queued(bool queued) {
        queued_ = queued;
      }

      /*
        Invoke/signal the diffrent TCP events.
      */
      inline bool signal_accept() { return on_accept_(shared_from_this()); }

      inline void signal_connect() { on_connect_(shared_from_this()); }

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

      inline int32_t usable_window() const {
        auto x = (int64_t)control_block.SND.UNA + (int64_t)control_block.SND.WND - (int64_t)control_block.SND.NXT;
        return (int32_t) x;
      }

      /// Congestion Control [RFC 5681] ///

      inline uint16_t SMSS() const {
        return host_.MSS();
      }

      inline uint16_t RMSS() const {
        return control_block.SND.MSS;
      }

      inline int32_t flight_size() const {
        return control_block.SND.NXT - control_block.SND.UNA;
      }

      inline void init_cwnd() {
        control_block.SND.cwnd = 10*SMSS();
      }

      inline void reduce_slow_start_threshold() {
        control_block.ssthresh = std::max( (flight_size() / 2), (2 * SMSS()) );
        printf("TCP::Connection::reduce_slow_start_threshold> Slow start threshold reduced: %u\n",
          control_block.ssthresh);
      }

      inline void segment_loss_detected() {
        reduce_slow_start_threshold();
      }
      /*

      */
      size_t duplicate_ack(TCP::Seq ack);

      /*
        Generate a new ISS.
      */
      TCP::Seq generate_iss();


      /// STATE HANDLING ///
      /*
        Set state. (used by substates)
      */
      void set_state(State& state);

      /*
        Transmit the send buffer.
      */
      void transmit();

      /*
        Transmit the packet and hooks up retransmission.
      */
      void transmit(TCP::Packet_ptr);

      /*
        Retransmit the packet.
      */
      void retransmit(TCP::Packet_ptr);

      /*
        Creates a new outgoing packet with the current TCB values and options.
      */
      TCP::Packet_ptr create_outgoing_packet();

      /*
       */
      inline TCP::Packet_ptr outgoing_packet() {
        return create_outgoing_packet();
      }


      /// RETRANSMISSION ///

      /*
        Starts a retransmission timer that retransmits the packet when RTO has passed.

        // TODO: Calculate RTO, currently hardcoded to 1 second (1000ms).
        */
      void queue_retransmission(TCP::Packet_ptr, size_t rt_attempt = 1);

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

      /*
        Receive a TCP Packet.
      */
      void segment_arrived(TCP::Packet_ptr);

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

    std::queue<Connection_ptr> write_queue;

    /*
      Settings
    */
    TCP::Port current_ephemeral_ = 1024;

    std::chrono::milliseconds MAX_SEG_LIFETIME;

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

    /*
      Process the write queue with the given amount of free packets.
    */
    void process_write_queue(size_t packets);

    /*
      Ask to send a Connection's WriteBuffer.
      If there is no free packets, the job will be queued.
    */
    size_t send(Connection_ptr, Connection::WriteBuffer&);

    /*
      Force the TCP to process the it's queue with the current amount of available packets.
    */
    inline void kick() {
      process_write_queue(inet_.transmit_queue_available());
    }


  }; // < class TCP

}; // < namespace net

#endif
