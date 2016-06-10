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
#include <bitset>

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

      inline TCP::Header& tcp_header() const
      { return *(TCP::Header*) ip_data(); }

      //! initializes to a default, empty TCP packet, given
      //! a valid MTU-sized buffer
      void init()
      {
        PacketIP4::init();

        // clear TCP headers
        memset(ip_data(), 0, sizeof(TCP::Header));

        set_protocol(IP4::IP4_TCP);
        set_win(TCP::default_window_size);
        set_offset(5);
        set_length();

        // set TCP payload location (!?)
        set_payload(buffer() + tcp_full_header_length());

        debug2("<TCP::Packet::init> size()=%u ip_header_size()=%u full_header_size()=%u\n",
          size(), ip_header_size(), tcp_full_header_length());
      }

      // GETTERS
      inline TCP::Port src_port() const
      { return ntohs(tcp_header().source_port); }

      inline TCP::Port dst_port() const
      { return ntohs(tcp_header().destination_port); }

      inline TCP::Seq seq() const
      { return ntohl(tcp_header().seq_nr); }

      inline TCP::Seq ack() const
      { return ntohl(tcp_header().ack_nr); }

      inline uint16_t win() const
      { return ntohs(tcp_header().window_size); }

      inline TCP::Socket source() const
      { return TCP::Socket{src(), src_port()}; }

      inline TCP::Socket destination() const
      { return TCP::Socket{dst(), dst_port()}; }

      inline TCP::Seq end() const
      { return seq() + tcp_data_length(); }

      // SETTERS
      inline TCP::Packet& set_src_port(TCP::Port p) {
        tcp_header().source_port = htons(p);
        return *this;
      }

      inline TCP::Packet& set_dst_port(TCP::Port p) {
        tcp_header().destination_port = htons(p);
        return *this;
      }

      inline TCP::Packet& set_seq(TCP::Seq n) {
        tcp_header().seq_nr = htonl(n);
        return *this;
      }

      inline TCP::Packet& set_ack(TCP::Seq n) {
        tcp_header().ack_nr = htonl(n);
        return *this;
      }

      inline TCP::Packet& set_win(uint16_t size) {
        tcp_header().window_size = htons(size);
        return *this;
      }

      inline TCP::Packet& set_checksum(uint16_t checksum) {
        tcp_header().checksum = checksum;
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
        tcp_header().offset_flags.whole |= htons(f);
        return *this;
      }

      inline TCP::Packet& set_flags(uint16_t f) {
        tcp_header().offset_flags.whole |= htons(f);
        return *this;
      }

      inline TCP::Packet& clear_flag(TCP::Flag f) {
        tcp_header().offset_flags.whole &= ~ htons(f);
        return *this;
      }

      inline TCP::Packet& clear_flags() {
        tcp_header().offset_flags.whole &= 0x00ff;
        return *this;
      }

      inline bool isset(TCP::Flag f) const
      { return ntohs(tcp_header().offset_flags.whole) & f; }

      //TCP::Flag flags() const { return (htons(tcp_header().offset_flags.whole) << 8) & 0xFF; }


      /// OFFSET, OPTIONS, DATA ///

      // Get the raw tcp offset, in quadruples
      inline uint8_t offset() const
      { return (uint8_t)(tcp_header().offset_flags.offset_reserved >> 4); }

      // Set raw TCP offset in quadruples
      inline void set_offset(uint8_t offset)
      { tcp_header().offset_flags.offset_reserved = (offset << 4); }

      // The actual TCP header size (including options).
      inline uint8_t tcp_header_length() const
      { return offset() * 4; }

      inline uint8_t tcp_full_header_length() const
      { return ip_full_header_length() + tcp_header_length(); }

      // The total length of the TCP segment (TCP header + data)
      uint16_t tcp_length() const
      { return tcp_header_length() + tcp_data_length(); }

      // Where data starts
      inline char* tcp_data()
      { return ip_data() + tcp_header_length(); }

      // Length of data in packet when header has been accounted for
      inline uint16_t tcp_data_length() const
      { return ip_data_length() - tcp_header_length(); }

      inline bool has_tcp_data() const
      { return tcp_data_length() > 0; }

      template <typename T, typename... Args>
      inline void add_tcp_option(Args&&... args) {
        // to avoid headache, options need to be added BEFORE any data.
        assert(!has_tcp_data());
        // option address
        auto* addr = tcp_options()+tcp_options_length();
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

      // Options
      inline uint8_t* tcp_options()
      { return (uint8_t*) tcp_header().options; }

      inline uint8_t tcp_options_length() const
      { return tcp_header_length() - sizeof(TCP::Header); }

      inline bool has_tcp_options() const
      { return tcp_options_length() > 0; }


      //! assuming the packet has been properly initialized,
      //! this will fill bytes from @buffer into this packets buffer,
      //! then return the number of bytes written. buffer is unmodified
      size_t fill(const char* buffer, size_t length) {
        size_t rem = capacity() - size();
        size_t total = (length < rem) ? length : rem;
        // copy from buffer to packet buffer
        memcpy(tcp_data() + tcp_data_length(), buffer, total);
        // set new packet length
        set_length(tcp_data_length() + total);
        return total;
      }

      /// HELPERS ///

      bool is_acked_by(const Seq ack) const
      { return ack >= (seq() + tcp_data_length()); }

      bool should_rtx() const
      { return has_tcp_data() or isset(SYN) or isset(FIN); }

      inline std::string to_string() {
        std::ostringstream os;
        os << "[ S:" << source().to_string() << " D:" <<  destination().to_string()
           << " SEQ:" << seq() << " ACK:" << ack()
           << " HEAD-LEN:" << (int)tcp_header_length() << " OPT-LEN:" << (int)tcp_options_length() << " DATA-LEN:" << tcp_data_length()
           << " WIN:" << win() << " FLAGS:" << std::bitset<8>{tcp_header().offset_flags.flags}  << " ]";
        return os.str();
      }


    private:
      // sets the correct length for all the protocols up to IP4
      void set_length(uint16_t newlen = 0) {
        // new total packet length
        set_size( tcp_full_header_length() + newlen );
        // update IP packet aswell - bad idea?
        set_segment_length();
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

        void clear() {
          memset(begin(), 0, offset);
          remaining = capacity();
          offset = 0;
          push = false;
        }

        /*
          Renews the ReadBuffer by assigning a new buffer_t, releasing ownership
        */
        inline void renew() {
          remaining = capacity();
          offset = 0;
          buffer = buffer_t(new uint8_t[remaining], std::default_delete<uint8_t[]>());
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
        size_t acknowledged;
        bool push;

        WriteBuffer(buffer_t buf, size_t length, bool PSH, size_t offs = 0)
          : buffer(buf), remaining(length-offs), offset(offs), acknowledged(0), push(PSH) {}

        inline size_t length() const { return remaining + offset; }

        inline bool done() const { return acknowledged == length(); }

        inline uint8_t* begin() const { return buffer.get(); }

        inline uint8_t* pos() const { return buffer.get() + offset; }

        inline uint8_t* end() const { return buffer.get() + length(); }

        inline bool advance(size_t length) {
          assert(length <= remaining);
          offset += length;
          remaining -= length;
          return length > 0;
        }

        size_t acknowledge(size_t bytes) {
          auto acked = std::min(bytes, length()-acknowledged);
          acknowledged += acked;
          return acked;
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
        Write Queue containig WriteRequests from user.
        Stores requests until they are fully acknowledged;
        this will make it possible to retransmit
      */
      struct WriteQueue {

        std::deque<WriteRequest> q;

        /* Current element (index + 1) */
        uint32_t current;

        WriteQueue() : q(), current(0) {}

        /*
          Acknowledge n bytes from the write queue.
          If a Request is fully acknowledged, release from queue
          and "step back".
        */
        void acknowledge(size_t bytes) {
          debug2("<Connection::WriteQueue> Acknowledge %u bytes\n",bytes);
          while(bytes and !q.empty())
          {
            auto& buf = q.front().first;

            bytes -= buf.acknowledge(bytes);
            if(buf.done()) {
              q.pop_front();
              current--;
            }
          }
        }

        bool empty() const
        { return q.empty(); }

        size_t size() const
        { return q.size(); }

        /*
          If the queue has more data to send
        */
        bool remaining_requests() const
        { return !q.empty() and q.back().first.remaining; }

        /*
          The current buffer to write from.
          Can be in the middle/back of the queue due to unacknowledged buffers in front.
        */
        const WriteBuffer& nxt()
        { return q[current-1].first; }

        /*
          The oldest unacknowledged buffer. (Always in front)
        */
        const WriteBuffer& una()
        { return q.front().first; }

        /*
          Advances the queue forward.
          If current buffer finishes; exec user callback and step to next.
        */
        void advance(size_t bytes) {

          auto& buf = q[current-1].first;
          buf.advance(bytes);

          debug2("<Connection::WriteQueue> Advance: bytes=%u off=%u rem=%u ack=%u\n",
            bytes, buf.offset, buf.remaining, buf.acknowledged);

          if(!buf.remaining) {
            debug("<Connection::WriteQueue> Advance: Done (%u)\n",
              buf.offset);
            // make sure to advance current before callback is made,
            // but after index (current) is received.
            q[current++-1].second(buf.offset);
          }
        }

        /*
          Add a request to the back of the queue.
          If the queue was empty/finished, point current to the new request.
        */
        void push_back(const WriteRequest& wr) {
          q.push_back(wr);
          debug("<Connection::WriteQueue> Inserted WR: off=%u rem=%u ack=%u\n",
            wr.first.offset, wr.first.remaining, wr.first.acknowledged);
          if(current == q.size()-1)
            current++;
        }

        /*
          Remove all write requests from queue and signal how much was written for each request.
        */
        void reset() {
          while(!q.empty()) {
            auto& req = q.front();
            // only give callbacks on request who hasnt finished writing
            // (others has already been called)
            if(req.first.remaining > 0)
              req.second(req.first.offset);
            q.pop_front();
          }
        }
      }; // < TCP::Connection::WriteQueue


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

      // TODO: Remove reference to Connection, probably not needed..
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
            return "Connection refused";
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
          CLOSED = -1, // This inditactes that a Connection is done and should be closed.
          OK = 0, // Does nothing
          CLOSE = 1 // This indicates that the CLIENT (peer) has/wants to close their end.
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
        uint32_t cwnd; // Congestion window [RFC 5681]
        Seq recover; // New Reno [RFC 6582]

        TCB() {
          SND = { 0, 0, TCP::default_window_size, 0, 0, 0, TCP::default_mss };
          ISS = (Seq)4815162342;
          RCV = { 0, TCP::default_window_size, 0, 0 };
          IRS = 0;
          ssthresh = TCP::default_window_size;
          cwnd = 0;
          recover = 0;
        };

        void init() {
          ISS = TCP::generate_iss();
          recover = ISS; // [RFC 6582]
        }

        bool slow_start() {
          return cwnd < ssthresh;
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
        The local Port bound to this connection.
      */
      inline TCP::Port local_port() const
      { return local_port_; }

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
      inline void write(const void* buf, size_t n, WriteCallback callback, bool PUSH) {
        auto buffer = buffer_t(new uint8_t[n], std::default_delete<uint8_t[]>());
        memcpy(buffer.get(), buf, n);
        write(buffer, n, callback, PUSH);
      }

      inline void write(const void* buf, size_t n, WriteCallback callback)
      { write(buf, n, callback, true); }

      // results in ambiguous call to member function
      //inline void write(const void* buf, size_t n, bool PUSH)
      //{ write(buf, n, WriteCallback::from<Connection,&Connection::default_on_write>(this), PUSH); }

      inline void write(const void* buf, size_t n)
      { write(buf, n, WriteCallback::from<Connection,&Connection::default_on_write>(this), true); }

      /*
        Works as write(const void*, size_t, WriteCallback, bool),
        but with the exception of avoiding copying the data to an internal buffer.
      */
      inline void write(buffer_t buffer, size_t n, WriteCallback callback, bool PUSH)
      { write({buffer, n, PUSH}, callback); }

      inline void write(buffer_t buffer, size_t n, WriteCallback callback)
      { write({buffer, n, true}, callback); }

      // results in ambiguous call to member function
      //inline void write(buffer_t buffer, size_t n, bool PUSH)
      //{ write({buffer, n, PUSH}, WriteCallback::from<Connection,&Connection::default_on_write>(this)); }

      inline void write(buffer_t buffer, size_t n)
      { write({buffer, n, true}, WriteCallback::from<Connection,&Connection::default_on_write>(this)); }

      /*
        Write a WriteBuffer asynchronous to a remote and calls the WriteCallback when done (or aborted).
      */
      void write(WriteBuffer request, WriteCallback callback);

      inline void default_on_write(size_t) {};


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
        return cb.SND.NXT - cb.ISS;
      }

      /*
        Calculates and return bytes received.
        TODO: Not sure if this will suffice.
      */
      inline uint32_t bytes_received() const {
        return cb.RCV.NXT - cb.IRS;
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
      TCB cb;                // 36 B

      /*
        The given read request
      */
      ReadRequest read_request;

      /*
        Queue for write requests to process
      */
      WriteQueue writeq;

      /*
        State if connection is in TCP write queue or not.
      */
      bool queued_;

      struct {
        hw::PIT::Timer_iterator iter;
        bool active = false;
        size_t i = 0;
      } rtx_timer;

      /*
        When time-wait timer was started.
      */
      uint64_t time_wait_started;



      // [RFC 6298]
      // Round Trip Time Measurer
      struct RTTM {
        using timestamp_t = double;
        using duration_t = double;

        // clock granularity
        //static constexpr duration_t CLOCK_G = hw::PIT::frequency().count() / 1000;
        static constexpr duration_t CLOCK_G = 0.0011;

        static constexpr double K = 4.0;

        static constexpr double alpha = 1.0/8;
        static constexpr double beta = 1.0/4;

        timestamp_t t; // tick when measure is started

        duration_t SRTT; // smoothed round-trip time
        duration_t RTTVAR; // round-trip time variation
        duration_t RTO; // retransmission timeout

        bool active = false;

        RTTM() : t(OS::uptime()), SRTT(1.0), RTTVAR(1.0), RTO(1.0), active(false) {}

        void start() {
          t = OS::uptime();
          active = true;
        }

        void stop(bool first = false) {
          assert(active);
          active = false;
          // round trip time (RTT)
          auto rtt = OS::uptime() - t;
          debug2("<TCP::Connection::RTT> RTT: %ums\n",
            (uint32_t)(rtt * 1000));
          if(!first)
            sub_rtt_measurement(rtt);
          else {
            first_rtt_measurement(rtt);
          }
        }

        /*
          When the first RTT measurement R is made, the host MUST set

          SRTT <- R
          RTTVAR <- R/2
          RTO <- SRTT + max (G, K*RTTVAR)

          where K = 4.
        */
        inline void first_rtt_measurement(duration_t R) {
          SRTT = R;
          RTTVAR = R/2;
          update_rto();
        }

        /*
          When a subsequent RTT measurement R' is made, a host MUST set

          RTTVAR <- (1 - beta) * RTTVAR + beta * |SRTT - R'|
          SRTT <- (1 - alpha) * SRTT + alpha * R'

          The value of SRTT used in the update to RTTVAR is its value
          before updating SRTT itself using the second assignment.  That
          is, updating RTTVAR and SRTT MUST be computed in the above
          order.

          The above SHOULD be computed using alpha=1/8 and beta=1/4 (as
          suggested in [JK88]).

          After the computation, a host MUST update
          RTO <- SRTT + max (G, K*RTTVAR)
        */
        inline void sub_rtt_measurement(duration_t R) {
          RTTVAR = (1 - beta) * RTTVAR + beta * std::abs(SRTT-R);
          SRTT = (1 - alpha) * SRTT + alpha * R;
          update_rto();
        }

        inline void update_rto() {
          RTO = std::max(SRTT + std::max(CLOCK_G, K * RTTVAR), 1.0);
          debug2("<TCP::Connection::RTO> RTO updated: %ums\n",
            (uint32_t)(RTO * 1000));
        }

      } rttm;


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
        return host_.send(shared_from_this(), (char*)buffer.pos(), buffer.remaining);
      }

      /*
        Segmentize buffer into packets until either everything has been written,
        or all packets are used up.
      */
      size_t send(const char* buffer, size_t remaining, size_t& packets);

      inline size_t send(WriteBuffer& buffer, size_t& packets, size_t n) {
        return send((char*)buffer.pos(), n, packets);
      }

      /*
        Process the write queue with the given amount of packets.
      */
      void offer(size_t& packets);

      /*
        Returns if the connection has a doable write job.
      */
      inline bool has_doable_job() {
        return writeq.remaining_requests() and usable_window() >= SMSS();
      }

      /*
        Try to process the current write queue.
      */
      void writeq_push();

      /*
        Try to write (some of) queue on connected.
      */
      inline void writeq_on_connect() { writeq_push(); }

      /*
        Reset queue on disconnect. Clears the queue and notice every requests callback.
      */
      void writeq_reset();

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


      // RFC 3042
      void limited_tx();

      /// TCB HANDLING ///

      /*
        Returns the TCB.
      */
      inline Connection::TCB& tcb() { return cb; }

      /*

        SND.UNA + SND.WND - SND.NXT
        SND.UNA + WINDOW - SND.NXT
      */
      inline uint32_t usable_window() const {
        auto x = (int64_t)send_window() - (int64_t)flight_size();
        return (uint32_t) std::max(0ll, x);
      }

      /*

        Note:
        Made a function due to future use when Window Scaling Option is added.
      */
      inline uint32_t send_window() const {
        return std::min((uint32_t)cb.SND.WND, cb.cwnd);
      }

      inline int32_t congestion_window() const {
        auto win = (uint64_t)cb.SND.UNA + std::min((uint64_t)cb.cwnd, (uint64_t)send_window());
        return (int32_t)win;
      }

      /*
        Acknowledge a packet
        - TCB update, Congestion control handling, RTT calculation and RT handling.
      */
      bool handle_ack(TCP::Packet_ptr);

      /*
        When a duplicate ACK is received.
      */
      void on_dup_ack();

      /*
        Is it possible to send ONE segment.
      */
      bool can_send_one();

      /*
        Is the usable window large enough, and is there data to send.
      */
      bool can_send();

      /*
        Send as much as possible from write queue.
      */
      void send_much();

      /*
        Fill a packet with data and give it a SEQ number.
      */
      size_t fill_packet(Packet_ptr, const char*, size_t, Seq);

      /// Congestion Control [RFC 5681] ///

      // is fast recovery state
      bool fast_recovery = false;

      // First partial ack seen
      bool reno_fpack_seen = false;

      // limited transmit [RFC 3042] active
      bool limited_tx_ = true;

      // number of duplicate acks
      size_t dup_acks_ = 0;

      Seq prev_highest_ack_ = 0;
      Seq highest_ack_ = 0;

      // number of non duplicate acks received
      size_t acks_rcvd_ = 0;

      inline void setup_congestion_control()
      { reno_init(); }

      inline uint16_t SMSS() const
      { return host_.MSS(); }

      inline uint16_t RMSS() const
      { return cb.SND.MSS; }

      inline uint32_t flight_size() const
      { return (uint64_t)cb.SND.NXT - (uint64_t)cb.SND.UNA; }

      /// Reno ///

      inline void reno_init() {
        reno_init_cwnd(3);
        reno_init_sshtresh();
      }

      inline void reno_init_cwnd(size_t segments)
      {
        cb.cwnd = segments*SMSS();
        debug2("<TCP::Connection::reno_init_cwnd> Cwnd initilized: %u\n", cb.cwnd);
      }

      inline void reno_init_sshtresh()
      { cb.ssthresh = cb.SND.WND; }


      inline void reno_increase_cwnd(uint16_t n)
      { cb.cwnd += std::min(n, SMSS()); }

      inline void reno_deflate_cwnd(uint16_t n)
      { cb.cwnd -= (n >= SMSS()) ? n-SMSS() : n; }

      inline void reduce_ssthresh() {
        auto fs = flight_size();
        debug2("<Connection::reduce_ssthresh> FlightSize: %u\n", fs);

        auto two_seg = 2*(uint32_t)SMSS();

        if(limited_tx_)
          fs = (fs >= two_seg) ? fs - two_seg : 0;

        cb.ssthresh = std::max( (fs / 2), two_seg );
        debug2("<TCP::Connection::reduce_ssthresh> Slow start threshold reduced: %u\n",
          cb.ssthresh);
      }

      inline void fast_retransmit() {
        debug("<TCP::Connection::fast_retransmit> Fast retransmit initiated.\n");
        // reduce sshtresh
        reduce_ssthresh();
        // retransmit segment starting SND.UNA
        retransmit();
        // inflate congestion window with the 3 packets we got dup ack on.
        cb.cwnd = cb.ssthresh + 3*SMSS();
        fast_recovery = true;
      }

      inline void finish_fast_recovery() {
        reno_fpack_seen = false;
        fast_recovery = false;
        cb.cwnd = std::min(cb.ssthresh, std::max(flight_size(), (uint32_t)SMSS()) + SMSS());
        debug("<TCP::Connection::finish_fast_recovery> Finished Fast Recovery - Cwnd: %u\n", cb.cwnd);
      }

      inline bool reno_full_ack(Seq ACK)
      { return ACK - 1 > cb.recover; }

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
        Creates a new outgoing packet with the current TCB values and options.
      */
      TCP::Packet_ptr create_outgoing_packet();

      /*
      */
      inline TCP::Packet_ptr outgoing_packet()
      { return create_outgoing_packet(); }


      /// RETRANSMISSION ///

      /*
        Retransmit the first packet in retransmission queue.
      */
      void retransmit();

      /*
        Start retransmission timer.
      */
      void rtx_start();

      /*
        Stop retransmission timer.
      */
      void rtx_stop();

      /*
        Restart retransmission timer.
      */
      inline void rtx_reset() {
        rtx_stop();
        rtx_start();
      }

      /*
        Number of retransmission attempts on the packet first in RT-queue
      */
      size_t rto_attempt = 0;
      // number of retransmitted SYN packets.
      size_t syn_rtx_ = 0;

      /*
        Retransmission timeout limit reached
      */
      inline bool rto_limit_reached() const
      { return rto_attempt >= 15 or syn_rtx_ >= 5; };

      /*
        Remove all packets acknowledge by ACK in retransmission queue
      */
      void rtx_ack(Seq ack);

      /*
        Delete retransmission queue
      */
      void rtx_clear();

      /*
        When retransmission times out.
      */
      void rtx_timeout();


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
        return std::min(host_.MSS(), cb.SND.MSS) + sizeof(TCP::Header);
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
    */
    inline constexpr uint16_t MSS() const {
      return network().MDDS() - sizeof(TCP::Header);
    }

    /*
      Show all connections for TCP as a string.
    */
    std::string to_string() const;

    inline std::string status() const
    { return to_string(); }




  private:

    IPStack& inet_;
    std::map<TCP::Port, Connection> listeners_;
    std::map<Connection::Tuple, Connection_ptr> connections_;

    downstream _network_layer_out;

    std::deque<Connection_ptr> writeq;

    std::vector<Port> used_ports;

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
    static TCP::Seq generate_iss();

    /*
      Returns a free port for outgoing connections.
    */
    TCP::Port next_free_port();

    /*
      Check if the port is in use either among "listeners" or "connections"
    */
    bool port_in_use(const TCP::Port) const;

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
    void process_writeq(size_t packets);

    /*

    */
    size_t send(Connection_ptr, const char* buffer, size_t n);

    /*
      Force the TCP to process the it's queue with the current amount of available packets.
    */
    inline void kick() {
      process_writeq(inet_.transmit_queue_available());
    }

    inline IP4& network() const {
      return inet_.ip_obj();
    }


  }; // < class TCP

}; // < namespace net

#endif
