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

#pragma once
#ifndef NET_TCP_CONNECTION_HPP
#define NET_TCP_CONNECTION_HPP

#include "common.hpp"
#include "packet.hpp"
#include "read_request.hpp"
#include "rttm.hpp"
#include "socket.hpp"
#include "tcp_errors.hpp"
#include "write_queue.hpp"
#include <delegate>
#include <util/timer.hpp>

namespace net {
  class TCP;
}

namespace net {
namespace tcp {

/*
  A connection between two Sockets (local and remote).
  Receives and handle TCP::Packet.
  Transist between many states.
*/
class Connection : public std::enable_shared_from_this<Connection> {
  friend class net::TCP;
  friend class Listener;

public:
  /** Connection identifier */
  using Tuple = std::pair<port_t, Socket>;
  /** Interface for TCP states */
  class State;
  /** Disconnect event */
  struct Disconnect;

  using WriteBuffer = Write_queue::WriteBuffer;

public:

  /*
    On connected - When both hosts exchanged sequence numbers (handshake is done).
    Now in ESTABLISHED state - it's allowed to write and read to/from the remote.
  */
  using ConnectCallback         = delegate<void(Connection_ptr self)>;
  inline Connection&            on_connect(ConnectCallback);

  /** Supplied on read - called when a buffer is either full or */
  using ReadCallback            = delegate<void(buffer_t, size_t)>;
  inline Connection&            on_read(size_t recv_bufsz, ReadCallback);

  /*
    On disconnect - When a remote told it wanna close the connection.
    Connection has received a FIN, currently last thing that will happen before a connection is remoed.
  */

  // TODO: Remove reference to Connection, probably not needed..
  using DisconnectCallback      = delegate<void(Connection_ptr self, Disconnect)>;
  inline Connection&            on_disconnect(DisconnectCallback);

  /**
   * Emitted right before the connection gets cleaned up
   */
  using CloseCallback           = delegate<void()>;
  inline Connection&            on_close(CloseCallback);

  /**
   * Emitted every time the connection finishes writing a chunk (job)
   */
  using WriteCallback           = delegate<void(size_t)>;
  inline Connection&            on_write(WriteCallback);

  /*
    On error - When any of the users request fails.
  */
  using ErrorCallback           = delegate<void(TCPException)>;
  inline Connection&            on_error(ErrorCallback);

  /*
    When a packet is dropped - Everytime an incoming packet is unallowed, it will be dropped.
    Can be used for debugging.
  */
  using PacketDroppedCallback   = delegate<void(const Packet&, const std::string&)>;
  inline Connection&            on_packet_dropped(PacketDroppedCallback);

  /**
   * Emitted on RTO - When the retransmission timer times out, before retransmitting.
   * Gives the current attempt and the current timeout in seconds.
   */
  using RtxTimeoutCallback      = delegate<void(size_t no_attempts, double rto)>;
  inline Connection&            on_rtx_timeout(RtxTimeoutCallback);


  /**
   * @brief      Async write of a shared buffer with a length.
   *             Avoids any copy of the data into the internal buffer.
   *             Calls write(Chunk c).
   *
   * @param[in]  buffer  shared buffer
   * @param[in]  n       length
   */
  inline void write(buffer_t buffer, size_t n);

  /**
   * @brief      Async write of a chunk.
   *
   * @param[in]  c     A chunk
   */
  void write(Chunk c);

  /**
   * @brief      Asyn write of a data with a length.
   *             Copies data into an internal (shared) buffer.
   *
   * @param[in]  buf   data
   * @param[in]  n     length
   */
  inline void write(const void* buf, size_t n);

  /**
   * @brief      Async write of a string.
   *             Calls write(const void* buf, size_t n)
   *
   * @param[in]  str   The string
   */
  inline void write(const std::string& str);

  /*
    Close connection.
  */
  void close();

  /*
    Abort connection. (Same as Terminate)
  */
  inline void abort();


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

    operator Reason() const noexcept { return reason; }

    operator std::string() const noexcept { return to_string(); }

    bool operator ==(const Disconnect& dc) const { return reason == dc.reason; }

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
  }; // < struct Connection::Disconnect

  /*
    Represent the Connection as a string (STATUS).
    Local:Port Remote:Port (STATE)
  */
  std::string to_string() const
  { return {local().to_string() + " " + remote_.to_string() + " (" + state_->to_string() + ")"}; }

  /*
    Returns the current state of the connection.
  */
  const Connection::State& state() const
  { return *state_; }

  /*
    Returns the previous state of the connection.
  */
  const Connection::State& prev_state() const
  { return *prev_state_; }

  /**
   * @brief Total number of bytes in read buffer
   *
   * @return bytes not yet read
   */
  size_t readq_size() const
  { return read_request.buffer.size(); }

  /**
   * @brief Total number of bytes in send queue
   *
   * @return total bytes in send queue
   */
  uint32_t sendq_size() const
  { return writeq.bytes_total(); }

  /**
   * @brief Total number of bytes not yet sent
   *
   * @return bytes not yet sent
   */
  uint32_t sendq_remaining() const
  { return writeq.bytes_remaining(); }

  /*
    Is the usable window large enough, and is there data to send.
  */
  constexpr bool can_send() const
  { return (usable_window() >= SMSS()) and writeq.has_remaining_requests(); }

  /*
    Return the id (TUPLE) of the connection.
  */
  Connection::Tuple tuple() const
  { return {local_port_, remote_}; }

  /// --- State checks --- ///

  bool is_listening() const;

  bool is_connected() const
  { return state_->is_connected(); }

  bool is_writable() const
  { return state_->is_writable(); }

  bool is_readable() const
  { return state_->is_readable(); }

  bool is_closing() const
  { return state_->is_closing(); }

  bool is_closed() const
  { return state_->is_closed(); };

  /*
    Returns if the TCP has the Connection in write queue
  */
  bool is_queued() const
  { return queued_; }

  /*
    Helper function for state checks.
  */
  bool is_state(const State& state) const
  { return state_ == &state; }

  bool is_state(const std::string& state_str) const
  { return state_->to_string() == state_str; }

  /*
    The hosting TCP instance.
  */
  TCP& host() const
  { return host_; }

  /*
    The local Port bound to this connection.
  */
  port_t local_port() const
  { return local_port_; }

  /*
    The local Socket bound to this connection.
  */
  Socket local() const;

  /*
    The remote Socket bound to this connection.
  */
  Socket remote() const
  { return remote_; }


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

    /** Open a Connection [OPEN] */
    virtual void open(Connection&, bool active = false);

    /** Write to a Connection [SEND] */
    virtual size_t send(Connection&, WriteBuffer&);

    /** Read from a Connection [RECEIVE] */
    virtual void receive(Connection&, ReadBuffer&&);

    /** Close a Connection [CLOSE] */
    virtual void close(Connection&);

    /** Terminate a Connection [ABORT] */
    virtual void abort(Connection&);

    /** Handle a Packet [SEGMENT ARRIVES] */
    virtual Result handle(Connection&, Packet_ptr in) = 0;

    /** The current state represented as a string [STATUS] */
    virtual std::string to_string() const = 0;


    virtual bool is_connected() const
    { return false; }

    virtual bool is_writable() const
    { return false; }

    virtual bool is_readable() const
    { return false; }

    virtual bool is_closing() const
    { return false; }

    virtual bool is_closed() const
    { return false; }

  protected:
    /*
      Helper functions
      TODO: Clean up names.
    */
    virtual bool check_seq(Connection&, const Packet&);

    virtual void unallowed_syn_reset_connection(Connection&, const Packet&);

    virtual bool check_ack(Connection&, const Packet&);

    virtual void process_segment(Connection&, Packet&);

    virtual void process_fin(Connection&, const Packet&);

    virtual void send_reset(Connection&);

  }; // < class Connection::State

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
      seq_t UNA;    // send unacknowledged
      seq_t NXT;    // send next
      uint32_t WND; // send window
      uint16_t UP;  // send urgent pointer
      seq_t WL1;    // segment sequence number used for last window update
      seq_t WL2;    // segment acknowledgment number used for last window update

      uint16_t MSS; // Maximum segment size for outgoing segments.

      uint8_t  wind_shift; // WS factor
      bool     TS_OK;  // Use timestamp option
    } SND; // <<
    seq_t ISS;      // initial send sequence number

    /* Receive Sequence Variables */
    struct {
      seq_t NXT;    // receive next
      uint32_t WND; // receive window
      uint16_t UP;  // receive urgent pointer

      uint16_t rwnd; // receivers advertised window [RFC 5681]

      uint8_t  wind_shift; // WS factor
    } RCV; // <<
    seq_t IRS;      // initial receive sequence number

    uint32_t ssthresh; // slow start threshold [RFC 5681]
    uint32_t cwnd;     // Congestion window [RFC 5681]
    seq_t recover;     // New Reno [RFC 6582]

    uint32_t TS_recent; // Recent timestamp received from user [RFC 7323]

    TCB(const uint32_t recvwin);
    TCB();

    void init() {
      ISS = Connection::generate_iss();
      recover = ISS; // [RFC 6582]
    }

    bool slow_start()
    { return cwnd < ssthresh; }

    std::string to_string() const;
  }__attribute__((packed)); // < struct Connection::TCB

  /*
    Creates a connection with a remote.
  */
  Connection(TCP& host, port_t local_port, Socket remote, ConnectCallback callback = nullptr);

  Connection(const Connection&)             = delete;
  Connection(Connection&&)                  = delete;
  Connection& operator=(const Connection&)  = delete;
  Connection& operator=(Connection&&)       = delete;

  /*
    Open connection.
  */
  void open(bool active = false);

  /*
    Set remote Socket bound to this connection.
  */
  void set_remote(Socket remote)
  { remote_ = remote; }

  // ???
  void deserialize_from(void*);
  int  serialize_to(void*) const;

  /** Reset all callbacks back to default **/
  void reset_callbacks();

  /*
    Destroy the Connection.
    Clean up.
  */
  ~Connection();

private:
  /** "Parent" for Connection. */
  TCP& host_;

  /* End points. */
  port_t local_port_;
  Socket remote_;

  /** The current state the Connection is in. Handles most of the logic. */
  State* state_;
  // Previous state. Used to keep track of state transitions.
  State* prev_state_;

  /** Keep tracks of all sequence variables. */
  TCB cb;

  /** The given read request */
  ReadRequest read_request;

  /** Queue for write requests to process */
  Write_queue writeq;

  /** Round Trip Time Measurer */
  RTTM rttm;

  /** Callbacks */
  ConnectCallback         on_connect_;
  DisconnectCallback      on_disconnect_;
  ErrorCallback           on_error_;
  PacketDroppedCallback   on_packet_dropped_;
  RtxTimeoutCallback      on_rtx_timeout_;
  CloseCallback           on_close_;

  /** Retransmission timer */
  Timer rtx_timer;

  /** Time Wait / DACK timeout timer */
  Timer timewait_dack_timer;

  /** Number of retransmission attempts on the packet first in RT-queue */
  int8_t rtx_attempt_ = 0;

  /** number of retransmitted SYN packets. */
  int8_t syn_rtx_ = 0;

  /** State if connection is in TCP write queue or not. */
  bool queued_;

  /** Congestion control */
  // is fast recovery state
  bool fast_recovery = false;
  // First partial ack seen
  bool reno_fpack_seen = false;
  /** limited transmit [RFC 3042] active */
  bool limited_tx_ = true;
  // Number of current duplicate ACKs. Is reset for every new ACK.
  uint8_t dup_acks_ = 0;

  seq_t highest_ack_ = 0;
  seq_t prev_highest_ack_ = 0;

  /** Delayed ACK - number of seg received without ACKing */
  uint8_t  dack_{0};
  seq_t    last_ack_sent_;

  /// --- CALLBACKS --- ///

  /**
   * @brief Cleanup callback
   * @details This is called to make sure TCP/Listener doesn't hold any shared ptr of
   * the given connection. This is only for internal use, and not visible for the user.
   *
   * @param  Connection to be cleaned up
   */
  using CleanupCallback   = delegate<void(Connection_ptr self)>;
  CleanupCallback         _on_cleanup_;
  inline Connection&      _on_cleanup(CleanupCallback cb);

  void default_on_disconnect(Connection_ptr, Disconnect);

  /// --- READING --- ///

  /*
    Read asynchronous from a remote.
    Create n sized internal read buffer and callback for when data is received.
    Callback will be called until overwritten with a new read() or connection closes.
    Buffer is cleared for data after every reset.
  */
  void read(size_t n, ReadCallback callback) {
    read({new_shared_buffer(n), n}, callback);
  }

  /*
    Assign the connections receive buffer and callback for when data is received.
    Works as read(size_t, ReadCallback);
  */
  void read(buffer_t buffer, size_t n, ReadCallback callback)
  { read({buffer, n}, callback); }

  void read(ReadBuffer&& buffer, ReadCallback callback);

  /*
    Assign the read request (read buffer)
  */
  void receive(ReadBuffer&& buffer)
  { read_request = {buffer}; }

  /*
    Receive data into the current read requests buffer.
  */
  size_t receive(const uint8_t* data, size_t n, bool PUSH);

  /*
    Copy data into the ReadBuffer
  */
  size_t receive(ReadBuffer& buf, const uint8_t* data, size_t n) {
    auto received = std::min(n, buf.remaining);
    memcpy(buf.pos(), data, received); // Can we use move?
    return received;
  }

  /*
    Remote is closing, no more data will be received.
    Returns receive buffer to user.
  */
  void receive_disconnect();


  /// --- WRITING --- ///

  /*
    Process the write queue with the given amount of packets.
    Called by TCP.
  */
  void offer(size_t& packets);

  /*
    Returns if the connection has a doable write job.
  */
  bool has_doable_job() const
  { return writeq.has_remaining_requests() and usable_window() >= SMSS(); }

  /*
    Try to process the current write queue.
  */
  void writeq_push();

  /*
    Try to write (some of) queue on connected.
  */
  void writeq_on_connect()
  { writeq_push(); }

  /*
    Reset queue on disconnect. Clears the queue and notice every requests callback.
  */
  void writeq_reset();

  /*
    Mark wether the Connection is in TCP write queue or not.
  */
  void set_queued(bool queued)
  { queued_ = queued; }

  /**
   * @brief      Sends an acknowledgement.
   */
  void send_ack();

  /*
    Invoke/signal the diffrent TCP events.
  */
  void signal_connect()
  { if(on_connect_) on_connect_(shared_from_this()); }

  void signal_disconnect(Disconnect::Reason&& reason)
  { on_disconnect_(shared_from_this(), Disconnect{reason}); }

  void signal_error(TCPException error)
  { if(on_error_) on_error_(std::forward<TCPException>(error)); }

  void signal_packet_dropped(const Packet& packet, const std::string& reason)
  { if(on_packet_dropped_) on_packet_dropped_(packet, reason); }

  void signal_rtx_timeout()
  { if(on_rtx_timeout_) on_rtx_timeout_(rtx_attempt_+1, rttm.RTO); }

  /*
    Drop a packet. Used for debug/callback.
  */
  void drop(const Packet& packet, const std::string& reason);

  void drop(const Packet& packet)
  { drop(packet, "None given."); }


  // RFC 3042
  void limited_tx();

  /// TCB HANDLING ///

  /*
    Returns the TCB.
  */
  Connection::TCB& tcb()
  { return cb; }

  /*
    Generate a new ISS.
  */
  static seq_t generate_iss();

  /*

    SND.UNA + SND.WND - SND.NXT
    SND.UNA + WINDOW - SND.NXT
  */
  uint32_t usable_window() const {
    const auto x = (int64_t)send_window() - (int64_t)flight_size();
    return (uint32_t) std::max(0ll, x);
  }

  uint32_t send_window() const {
    return std::min(cb.SND.WND, cb.cwnd);
  }

  int32_t congestion_window() const {
    const auto win = (uint64_t)cb.SND.UNA + (uint64_t)send_window();
    return (int32_t)win;
  }

  uint32_t flight_size() const
  { return (uint64_t)cb.SND.NXT - (uint64_t)cb.SND.UNA; }

  bool uses_window_scaling() const;

  bool uses_timestamps() const;

  /// --- INCOMING / TRANSMISSION --- ///
  /*
    Receive a TCP Packet.
  */
  void segment_arrived(Packet_ptr);

  /*
    Acknowledge a packet
    - TCB update, Congestion control handling, RTT calculation and RT handling.
  */
  bool handle_ack(const Packet&);

  /*
    When a duplicate ACK is received.
  */
  void on_dup_ack();

  /*
    Is it possible to send ONE segment.
  */
  constexpr bool can_send_one() const
  { return send_window() >= SMSS() and writeq.has_remaining_requests(); }

  /*
    Send as much as possible from write queue.
  */
  void send_much()
  { writeq_push(); }

  /*
    Fill a packet with data and give it a SEQ number.
  */
  size_t fill_packet(Packet&, const uint8_t*, size_t);

  /*
    Transmit the packet and hooks up retransmission.
  */
  void transmit(Packet_ptr);

  /*
    Creates a new outgoing packet with the current TCB values and options.
  */
  Packet_ptr create_outgoing_packet();

  Packet_ptr outgoing_packet()
  { return create_outgoing_packet(); }

  /*
    Maximum Segment Data Size
    (Limit the size for outgoing packets)
  */
  uint16_t MSDS() const;


  /// --- Congestion Control [RFC 5681] --- ///

  void setup_congestion_control()
  { reno_init(); }

  uint16_t SMSS() const;

  uint16_t RMSS() const
  { return cb.SND.MSS; }

  // Reno specifics //

  void reno_init()
  {
    reno_init_cwnd(3);
    reno_init_sshtresh();
  }

  void reno_init_cwnd(const size_t segments)
  { cb.cwnd = segments*SMSS(); }

  void reno_init_sshtresh()
  { cb.ssthresh = cb.SND.WND; }

  void reno_increase_cwnd(const uint16_t n)
  { cb.cwnd += std::min(n, SMSS()); }

  void reno_deflate_cwnd(const uint16_t n)
  { cb.cwnd -= (n >= SMSS()) ? n-SMSS() : n; }

  void reduce_ssthresh();

  void fast_retransmit();

  void finish_fast_recovery();

  bool reno_full_ack(seq_t ACK)
  { return ACK - 1 > cb.recover; }



  /// --- STATE HANDLING --- ///
  /*
    Set state. (used by substates)
  */
  void set_state(State& state);


  /// --- RETRANSMISSION --- ///

  /*
    Retransmit the first packet in retransmission queue.
  */
  void retransmit();

  /*
    Start retransmission timer.
  */
  void rtx_start()
  { rtx_timer.start(rttm.rto_ms()); }

  /*
    Stop retransmission timer.
  */
  void rtx_stop()
  { rtx_timer.stop(); }

  /*
    Restart retransmission timer.
  */
  void rtx_reset()
  { rtx_timer.restart(rttm.rto_ms()); }

  /*
    Retransmission timeout limit reached
  */
  bool rto_limit_reached() const
  { return rtx_attempt_ >= 15 or syn_rtx_ >= 5; };

  /*
    Remove all packets acknowledge by ACK in retransmission queue
  */
  void rtx_ack(seq_t ack);

  /*
    Delete retransmission queue
  */
  void rtx_clear();

  /*
    When retransmission times out.
  */
  void rtx_timeout();

  /** Start the timewait timeout for 2*MSL */
  void timewait_start();

  /** Restart the timewait timer if active */
  void timewait_restart();

  /** When timewait timer times out */
  void timewait_timeout()
  { signal_close(); }

  /** Wether to use Delayed ACK or not */
  bool use_dack() const;

  /**
   * @brief      Called when the DACK timeout timesout.
   */
  void dack_timeout()
  { send_ack(); }

  /**
   * @brief      Starts the DACK timer.
   */
  void start_dack();

  /**
   * @brief      Stops the DACK timer.
   */
  void stop_dack()
  { timewait_dack_timer.stop(); }

  /*
    Tell the host (TCP) to delete this connection.
  */
  void signal_close();

  /**
   * @brief Clean up user callbacks
   * @details Removes all the user defined lambdas to avoid any potential
   * copies of a Connection_ptr to the this connection.
   */
  void clean_up();


  /// --- OPTIONS --- ///

  /*
    Parse and apply options.
  */
  void parse_options(Packet&);

  /*
    Add an option.
  */
  void add_option(Option::Kind, Packet&);


}; // < class Connection

} // < namespace net
} // < namespace tcp

#include "connection.inc"

#endif // < NET_TCP_CONNECTION_HPP
