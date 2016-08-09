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

#include <hw/pit.hpp>

#include "common.hpp"
#include "read_buffer.hpp"
#include "rttm.hpp"
#include "socket.hpp"
#include "tcp_errors.hpp"
#include "write_queue.hpp"


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

  /*
    Callback when a receive buffer receives either push or is full
    - Supplied on asynchronous read
  */
  using ReadCallback = delegate<void(buffer_t, size_t)>;

  using WriteCallback = WriteQueue::WriteCallback;

  using WriteRequest = WriteQueue::WriteRequest;


  struct ReadRequest {
    ReadBuffer buffer;
    ReadCallback callback;

    ReadRequest(ReadBuffer buf, ReadCallback cb)
      : buffer(buf), callback(cb)
    {}

    ReadRequest(size_t n = 0)
      : buffer(buffer_t(new uint8_t[n], std::default_delete<uint8_t[]>()), n),
        callback([](auto, auto){})
    {}

    void clean_up() {
      callback.reset();
    }
  };


  /*
    On connected - When both hosts exchanged sequence numbers (handshake is done).
    Now in ESTABLISHED state - it's allowed to write and read to/from the remote.
  */
  using ConnectCallback                     = delegate<void(Connection_ptr self)>;
  inline Connection& on_connect(ConnectCallback);

  /*
    On disconnect - When a remote told it wanna close the connection.
    Connection has received a FIN, currently last thing that will happen before a connection is remoed.
  */

  // TODO: Remove reference to Connection, probably not needed..
  using DisconnectCallback          = delegate<void(Connection_ptr self, Disconnect)>;
  inline Connection& on_disconnect(DisconnectCallback);

  /*
    On error - When any of the users request fails.
  */
  using ErrorCallback                         = delegate<void(TCPException)>;
  inline Connection& on_error(ErrorCallback);

  /*
    When a packet is received - Everytime a connection receives an incoming packet.
    Would probably be used for debugging.
    (Currently not in use)
  */
  using PacketReceivedCallback      = delegate<void(Packet_ptr)>;
  inline Connection& on_packet_received(PacketReceivedCallback);

  /*
    When a packet is dropped - Everytime an incoming packet is unallowed, it will be dropped.
    Can be used for debugging.
  */
  using PacketDroppedCallback               = delegate<void(Packet_ptr, std::string)>;
  inline Connection& on_packet_dropped(PacketDroppedCallback);

  /**
   * Emitted on RTO - When the retransmission timer times out, before retransmitting.
   * Gives the current attempt and the current timeout in seconds.
   */
  using RtxTimeoutCallback                = delegate<void(size_t no_attempts, double rto)>;
  inline Connection& on_rtx_timeout(RtxTimeoutCallback);

  /**
   * Emitted right before the connection gets cleaned up
   */
  using CloseCallback                     = delegate<void()>;
  inline Connection& on_close(CloseCallback);

  /**
   * @brief Cleanup callback
   * @details This is called to make sure TCP/Listener doesn't hold any shared ptr of
   * the given connection. This is only for internal use, and not visible for the user.
   *
   * @param  Connection to be cleaned up
   */
  using CleanupCallback                   = delegate<void(Connection_ptr self)>;
  inline Connection& _on_cleanup(CleanupCallback);


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
  }; // < struct Connection::Disconnect

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
    virtual void receive(Connection&, ReadBuffer&);

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

  protected:
    /*
      Helper functions
      TODO: Clean up names.
    */
    virtual bool check_seq(Connection&, Packet_ptr);

    virtual void unallowed_syn_reset_connection(Connection&, Packet_ptr);

    virtual bool check_ack(Connection&, Packet_ptr);

    virtual void process_segment(Connection&, Packet_ptr);

    virtual void process_fin(Connection&, Packet_ptr);

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
      uint16_t WND; // send window
      uint16_t UP;  // send urgent pointer
      seq_t WL1;    // segment sequence number used for last window update
      seq_t WL2;    // segment acknowledgment number used for last window update

      uint16_t MSS; // Maximum segment size for outgoing segments.
    } SND; // <<
    seq_t ISS;      // initial send sequence number

    /* Receive Sequence Variables */
    struct {
      seq_t NXT;    // receive next
      uint16_t WND; // receive window
      uint16_t UP;  // receive urgent pointer

      uint16_t rwnd; // receivers advertised window [RFC 5681]
    } RCV; // <<
    seq_t IRS;      // initial receive sequence number

    uint32_t ssthresh; // slow start threshold [RFC 5681]
    uint32_t cwnd;     // Congestion window [RFC 5681]
    seq_t recover;     // New Reno [RFC 6582]

    TCB();

    void init();

    bool slow_start()
    { return cwnd < ssthresh; }

    std::string to_string() const;
  }__attribute__((packed)); // < struct Connection::TCB

  /*
    Creates a connection without a remote.
  */
  Connection(TCP& host, port_t local_port);

  /*
    Creates a connection with a remote.
  */
  Connection(TCP& host, port_t local_port, Socket remote);

  Connection(const Connection&) = default;

  /*
    The hosting TCP instance.
  */
  inline const TCP& host() const
  { return host_; }

  /*
    The local Port bound to this connection.
  */
  inline port_t local_port() const
  { return local_port_; }

  /*
    The local Socket bound to this connection.
  */
  Socket local() const;

  /*
    The remote Socket bound to this connection.
  */
  inline Socket remote() const
  { return remote_; }

  /*
    Set remote Socket bound to this connection.
    @WARNING: Should this be public? Used by TCP.
  */
  inline void set_remote(Socket remote)
  { remote_ = remote; }


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



  void setup_default_callbacks();

  /*
    Represent the Connection as a string (STATUS).
  */
  std::string to_string() const;

  /*
    Returns the current state of the connection.
  */
  inline Connection::State& state() const
  { return *state_; }

  /*
    Returns the previous state of the connection.
  */
  inline Connection::State& prev_state() const
  { return *prev_state_; }

  /*
    Calculates and return bytes transmitted.
    TODO: Not sure if this will suffice.
  */
  inline uint32_t bytes_transmitted() const
  { return 0; }

  /*
    Calculates and return bytes received.
    TODO: Not sure if this will suffice.
  */
  inline uint32_t bytes_received() const
  { return 0; }

  /*
    Bytes queued for transmission.
    TODO: Implement when retransmission is up and running.
  */
  //inline size_t send_queue_bytes() const {}

  /*
    Bytes currently in receive buffer.
  */
  inline size_t read_queue_bytes() const
  { return read_request.buffer.size(); }

  /*
    Return the id (TUPLE) of the connection.
  */
  inline Connection::Tuple tuple() const
  { return {local_port_, remote_}; }


  /*
    State checks.
  */
  bool is_listening() const;

  inline bool is_connected() const
  { return state_->is_connected(); }

  inline bool is_writable() const
  { return state_->is_writable(); }

  inline bool is_readable() const
  { return state_->is_readable(); }

  inline bool is_closing() const
  { return state_->is_closing(); }



  /*
    Helper function for state checks.
  */
  inline bool is_state(const State& state) const
  { return state_ == &state; }

  inline bool is_state(const std::string& state_str) const
  { return state_->to_string() == state_str; }

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
  port_t local_port_;    // 2 B
  Socket remote_;      // 8~ B

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

  RTTM rttm;

  /*
    State if connection is in TCP write queue or not.
  */
  bool queued_;

  struct {
    uint32_t id;
    bool active = false;
  } rtx_timer;

  /*
    When time-wait timer was started.
  */
  uint64_t time_wait_started;


  /// CALLBACK HANDLING ///

  /* When Connection is ESTABLISHED. */
  ConnectCallback on_connect_;
  void default_on_connect(Connection_ptr);

  /* When Connection is CLOSING. */
  DisconnectCallback on_disconnect_;
  void default_on_disconnect(Connection_ptr, Disconnect);

  /* When error occcured. */
  ErrorCallback on_error_;
  void default_on_error(TCPException);

  /* When packet is received */
  PacketReceivedCallback on_packet_received_;
  void default_on_packet_received(Packet_ptr);

  /* When a packet is dropped. */
  PacketDroppedCallback on_packet_dropped_;
  void default_on_packet_dropped(Packet_ptr, std::string);

  RtxTimeoutCallback on_rtx_timeout_;
  void default_on_rtx_timeout(size_t, double);

  CloseCallback on_close_;
  void default_on_close();

  CleanupCallback _on_cleanup_;
  void default_on_cleanup(Connection_ptr);

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
  void receive_disconnect();


  /// WRITING ///

  /*
    Active try to send a buffer by asking the TCP.
  */
  size_t send(WriteBuffer& buffer);

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
  inline bool has_doable_job();

  /*
    Try to process the current write queue.
  */
  void writeq_push();

  /*
    Try to write (some of) queue on connected.
  */
  inline void writeq_on_connect()
  { writeq_push(); }

  /*
    Reset queue on disconnect. Clears the queue and notice every requests callback.
  */
  void writeq_reset();

  /*
    Returns if the TCP has the Connection in write queue
  */
  inline bool is_queued() const
  { return queued_; }

  /*
    Mark wether the Connection is in TCP write queue or not.
  */
  inline void set_queued(bool queued)
  { queued_ = queued; }

  /*
    Invoke/signal the diffrent TCP events.
  */
  inline void signal_connect()
  { on_connect_(shared_from_this()); }

  inline void signal_disconnect(Disconnect::Reason&& reason)
  { on_disconnect_(shared_from_this(), Disconnect{reason}); }

  inline void signal_error(TCPException error)
  { on_error_(std::forward<TCPException>(error)); }

  inline void signal_packet_received(Packet_ptr packet)
  { on_packet_received_(packet); }

  inline void signal_packet_dropped(Packet_ptr packet, std::string reason)
  { on_packet_dropped_(packet, reason); }

  inline void signal_rtx_timeout()
  { on_rtx_timeout_(rtx_attempt_+1, rttm.RTO); }

  /*
    Drop a packet. Used for debug/callback.
  */
  inline void drop(Packet_ptr packet, std::string reason)
  { signal_packet_dropped(packet, reason); }
  inline void drop(Packet_ptr packet)
  { drop(packet, "None given."); }


  // RFC 3042
  void limited_tx();

  /// TCB HANDLING ///

  /*
    Returns the TCB.
  */
  inline Connection::TCB& tcb()
  { return cb; }

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
  bool handle_ack(Packet_ptr);

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
  size_t fill_packet(Packet_ptr, const char*, size_t, seq_t);

  /// Congestion Control [RFC 5681] ///

  // is fast recovery state
  bool fast_recovery = false;

  // First partial ack seen
  bool reno_fpack_seen = false;

  // limited transmit [RFC 3042] active
  bool limited_tx_ = true;

  // number of duplicate acks
  size_t dup_acks_ = 0;

  seq_t prev_highest_ack_ = 0;
  seq_t highest_ack_ = 0;

  // number of non duplicate acks received
  size_t acks_rcvd_ = 0;

  void setup_congestion_control();

  uint16_t SMSS() const;

  inline uint16_t RMSS() const
  { return cb.SND.MSS; }

  inline uint32_t flight_size() const
  { return (uint64_t)cb.SND.NXT - (uint64_t)cb.SND.UNA; }

  /// Reno ///

  void reno_init();

  void reno_init_cwnd(size_t segments);

  void reno_init_sshtresh()
  { cb.ssthresh = cb.SND.WND; }

  void reno_increase_cwnd(uint16_t n);

  void reno_deflate_cwnd(uint16_t n);

  void reduce_ssthresh();

  void fast_retransmit();

  void finish_fast_recovery();

  inline bool reno_full_ack(seq_t ACK)
  { return ACK - 1 > cb.recover; }

  /*
    Generate a new ISS.
  */
  seq_t generate_iss();


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
  void transmit(Packet_ptr);

  /*
    Creates a new outgoing packet with the current TCB values and options.
  */
  Packet_ptr create_outgoing_packet();

  /*
  */
  inline Packet_ptr outgoing_packet()
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
  size_t rtx_attempt_ = 0;
  // number of retransmitted SYN packets.
  size_t syn_rtx_ = 0;

  /*
    Retransmission timeout limit reached
  */
  inline bool rto_limit_reached() const
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
  void rtx_timeout(uint32_t);


  /*
    Start the time wait timeout for 2*MSL
  */
  void start_time_wait_timeout();

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


  /// OPTIONS ///
  /*
    Maximum Segment Data Size
    (Limit the size for outgoing packets)
  */
  inline uint16_t MSDS() const;

  /*
    Parse and apply options.
  */
  void parse_options(Packet_ptr);

  /*
    Add an option.
  */
  void add_option(Option::Kind, Packet_ptr);

  /*
    Receive a TCP Packet.
  */
  void segment_arrived(Packet_ptr);

}; // < class Connection

} // < namespace net
} // < namespace tcp

#include "connection.inc"

#endif // < NET_TCP_CONNECTION_HPP
