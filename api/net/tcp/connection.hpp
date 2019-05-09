// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015-2017 Oslo and Akershus University College of Applied Sciences
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
#include "packet_view.hpp"
#include "read_request.hpp"
#include "rttm.hpp"
#include "tcp_errors.hpp"
#include "write_queue.hpp"
#include "sack.hpp"

#include <net/socket.hpp>
#include <delegate>
#include <util/timer.hpp>

#include <util/alloc_pmr.hpp>

namespace net {
  // Forward declaration of the TCP object
  class TCP;
}

namespace net {
namespace tcp {

/*
  A connection between two Sockets (local and remote).
  Receives and handle TCP::Packet.
  Transist between many states.
*/
class Connection {
  friend class net::TCP;
  friend class Listener;

public:
  /** Connection identifier */
  using Tuple = std::pair<Socket, Socket>;
  /** Interface for TCP states */
  class State;
  /** Disconnect event */
  struct Disconnect;
  /** Reason for packet being dropped */
  enum class Drop_reason;

  using Byte = uint8_t;

  using WriteBuffer = Write_queue::WriteBuffer;

public:
  /** Called with the connection itself when it's been established. May be a nullptr if the connection failed. */
  using ConnectCallback         = delegate<void(Connection_ptr self)>;
  /**
   * @brief      Event when a connection has been established.
   *             This event lets you know when to start using the connection,
   *             and should always be assigned.
   *             NOTE: The Connection_ptr will be a nullptr when an outgoing connection failed.
   *
   * @param[in]  callback  The callback
   *
   * @return     This connection
   */
  inline Connection&            on_connect(ConnectCallback callback);

  /** Called with a shared buffer and the length of the data when received. */
  using ReadCallback            = delegate<void(buffer_t)>;
  /**
   * @brief      Event when incoming data is received by the connection.
   *             The recv_bufsz determines the size of the receive buffer.
   *             The callback is called when either 1) PSH is seen, or 2) the buffer is full
   *
   * @param[in]  recv_bufsz  The size of the receive buffer
   * @param[in]  callback    The callback
   *
   * @return     This connection
   */
  inline Connection&            on_read(size_t recv_bufsz, ReadCallback callback);


  using DataCallback           = delegate<void()>;
  /**
   * @brief      Event when incoming data is received by the connection.
   *             The callback is called when either 1) PSH is seen, or 2) the buffer is full
   *
   *             The user is expected to fetch data by calling read_next, otherwise the
   *             event will be triggered again. Unread data will be buffered as long as
   *             there is capacity in the read queue.
   *             If an on_read callback is also registered, this event has no effect.
   *
   * @param[in]  callback    The callback
   *
   * @return     This connection
   */
  inline Connection&            on_data(DataCallback callback);

  /**
   * @brief      Read the next fully acked chunk of received data if any.
   *
   * @return     Pointer to buffer if any, otherwise nullptr.
   */
  inline buffer_t read_next();

  /**
   * @return     The size of the next fully acked chunk of received data.
   */
  inline size_t   next_size();

  /** Called with the connection itself and the reason wrapped in a Disconnect struct. */
  using DisconnectCallback      = delegate<void(Connection_ptr self, Disconnect)>;
  /**
   * @brief      Event when a connection is being disconnected.
   *             This is when either 1) The peer has sent a FIN, indicating it wants to close,
   *             or 2) A RST is received telling the connection to reset
   *
   * @note       The default is to close the connection from our end as well.
   *             Remember to ::close() the connection inside this event (if that's what you want to do)
   *
   * @param[in]  callback  The callback
   *
   * @return     This connection
   */
  inline Connection&            on_disconnect(DisconnectCallback callback);

  /** Called with nothing ¯\_(ツ)_/¯ */
  using CloseCallback           = delegate<void()>;
  /**
   * @brief      Event when a connection is closing down.
   *             After this event has been called, the connection is useless.
   *             This is useful for cleaning up copies of the connection,
   *             and is more important than disconnect.
   *
   * @param[in]  callback  The callback
   *
   * @return     This connection
   */
  inline Connection&            on_close(CloseCallback callback);

  /** Called with the number of bytes written. */
  using WriteCallback           = delegate<void(size_t)>;
  /**
   * @brief      Event when a connection has finished sending a write request.
   *             This event does not tell if the data has been received by the peer,
   *             only that it has been transmitted.
   *             This can also be called with the amount written by the current request
   *             if a connection is aborted/closed with a non-empty queue.
   *
   * @param[in]  callback  The callback
   *
   * @return     This connection
   */
  inline Connection&            on_write(WriteCallback callback);

  /** Called with the packet that got dropped and the reason why. */
  using PacketDroppedCallback   = delegate<void(const Packet&, Drop_reason)>;

  /**
   * @brief      Only change the on_read callback without touching the buffer.
   *             Only useful in special cases. Assumes on_read has been called.
   *
   * @param[in]  callback  The callback
   *
   * @return     This connection
   */
  inline Connection&            set_on_read_callback(ReadCallback callback);


  /**
   * @brief      Async write of a shared buffer with a length.
   *             Avoids any copy of the data into the internal buffer.
   *
   * @param[in]  buffer  shared buffer
   * @param[in]  n       length
   */
  void write(buffer_t buffer);

  /**
   * @brief      Async write of a data with a length.
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

  /**
   * @brief      Async close of the connection, sending FIN.
   */
  void close();

  /**
   * @brief      Aborts the connection immediately, sending RST.
   */
  inline void abort();

  /**
   * @brief      Reason for disconnect event.
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

    std::string to_string() const noexcept {
      switch(reason)
      {
        case CLOSING:
          return "Connection closing";
        case REFUSED:
          return "Connection refused";
        case RESET:
          return "Connection reset";
        default:
          return "Unknown reason";
      } // < switch(reason)
    }
  }; // < struct Connection::Disconnect

  /**
   * Reason for packet being dropped.
   */
  enum class Drop_reason
  {
    NA, // N/A
    SEQ_OUT_OF_ORDER,
    ACK_NOT_SET,
    ACK_OUT_OF_ORDER,
    RCV_WND_ZERO,
    RST
  }; // < Drop_reason

  /**
   * @brief      Represent the Connection as a string (STATUS).
   *             Local:Port Remote:Port (STATE)
   *
   * @return     String representation of the object.
   */
  std::string to_string() const noexcept
  { return {local().to_string() + " " + remote_.to_string() + " (" + state_->to_string() + ")"}; }

  /**
   * @brief      Returns the current state of the connection.
   *
   * @return     The current state
   */
  const Connection::State& state() const noexcept
  { return *state_; }

  /**
   * @brief      Returns the previous state of the connection.
   *
   * @return     The previous state
   */
  const Connection::State& prev_state() const noexcept
  { return *prev_state_; }

  /**
   * @brief Total number of bytes in read buffer
   *
   * @return bytes not yet read
   */
  size_t readq_size() const
  { return (read_request) ? read_request->size() : 0; }

  /**
   * @brief Total number of bytes in send queue
   *
   * @return total bytes in send queue
   */
  uint32_t sendq_size() const noexcept
  { return writeq.bytes_total(); }

  /**
   * @brief Total number of bytes not yet sent
   *
   * @return bytes not yet sent
   */
  uint32_t sendq_remaining() const noexcept
  { return writeq.bytes_remaining(); }

  /**
   * @brief      Determines ability to send.
   *             Is the usable window large enough, and is there data to send.
   *
   * @return     True if able to send, False otherwise.
   */
  bool can_send() const noexcept
  { return (usable_window() >= SMSS()) and writeq.has_remaining_requests(); }

  /**
   * @brief      Return the "tuple" (id) of the connection.
   *             This is not a real tuple since the ip of the local socket
   *             is decided by the network stack.
   *
   * @return     A "tuple" of [[local port], [remote ip, remote port]]
   */
  Connection::Tuple tuple() const noexcept
  { return {local_, remote_}; }

  /// --- State checks --- ///

  /**
   * @brief      Determines if listening.
   *
   * @return     True if listening, False otherwise.
   */
  bool is_listening() const noexcept;

  /**
   * @brief      Determines if connected (established).
   *
   * @return     True if connected, False otherwise.
   */
  bool is_connected() const noexcept
  { return state_->is_connected(); }

  /**
   * @brief      Determines if writable. (write is allowed)
   *
   * @return     True if writable, False otherwise.
   */
  bool is_writable() const noexcept
  { return state_->is_writable(); }

  /**
   * @brief      Determines if readable. (data can be received)
   *
   * @return     True if readable, False otherwise.
   */
  bool is_readable() const noexcept
  { return state_->is_readable(); }

  /**
   * @brief      Determines if closing.
   *
   * @return     True if closing, False otherwise.
   */
  bool is_closing() const noexcept
  { return state_->is_closing(); }

  /**
   * @brief      Determines if closed.
   *
   * @return     True if closed, False otherwise.
   */
  bool is_closed() const noexcept
  { return state_->is_closed(); };

  /**
   * @brief      Determines if the TCP has the Connection in its write queue
   *
   * @return     True if queued, False otherwise.
   */
  bool is_queued() const noexcept
  { return queued_; }

  /**
   * @brief      Helper function for state checks.
   *
   * @param[in]  state  The state to be checked for
   *
   * @return     True if state, False otherwise.
   */
  bool is_state(const State& state) const noexcept
  { return state_ == &state; }

  /**
   * @brief      Helper function for state checks.
   *
   * @param[in]  state_str  The state string to be checked for
   *
   * @return     True if state, False otherwise.
   */
  bool is_state(const std::string& state_str) const noexcept
  { return state_->to_string() == state_str; }

  /**
   * @brief      The "hosting" TCP instance. The TCP object that the Connection is handled by.
   *
   * @return     A TCP object
   */
  TCP& host() noexcept
  { return host_; }

  /**
   * @brief      The local port bound to this connection.
   *
   * @return     A 16 bit unsigned port number
   */
  port_t local_port() const noexcept
  { return local_.port(); }

  /**
   * @brief      The local Socket bound to this connection.
   *
   * @return     A TCP Socket
   */
  const Socket& local() const noexcept
  { return local_; }

  /**
   * @brief      The remote Socket bound to this connection.
   *
   * @return     A TCP Socket
   */
  const Socket& remote() const noexcept
  { return remote_; }

  Protocol ipv() const noexcept
  { return is_ipv6_ ? Protocol::IPv6 : Protocol::IPv4; }

  auto bytes_sacked() const noexcept
  { return bytes_sacked_; }


  /**
   * @brief      Interface for one of the many states a Connection can have.
   *             Based on RFC 793
   */
  class State {
  public:
    enum Result {
      CLOSED = -1,  // This inditactes that a Connection is done and should be closed.
      OK = 0,       // Keep on processing
    };

    /** Open a Connection [OPEN] */
    virtual void open(Connection&, bool active = false);

    /** Write to a Connection [SEND] */
    virtual size_t send(Connection&, WriteBuffer&);

    /** Close a Connection [CLOSE] */
    virtual void close(Connection&);

    /** Terminate a Connection [ABORT] */
    virtual void abort(Connection&);

    /** Handle a Packet [SEGMENT ARRIVES] */
    virtual Result handle(Connection&, Packet_view& in) = 0;

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
    virtual bool check_seq(Connection&, Packet_view&);

    virtual void unallowed_syn_reset_connection(Connection&, const Packet_view&);

    virtual bool check_ack(Connection&, const Packet_view&);

    virtual void process_fin(Connection&, const Packet_view&);

    virtual void send_reset(Connection&);

  }; // < class Connection::State

  // Forward declaration of concrete states.
  // Definition in "tcp_connection_states.hpp"
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

  /**
   * @brief      Transmission Control Block.
   *             Keep tracks of all the data for a connection.

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

    TCB(const uint16_t mss, const uint32_t recvwin);
    TCB(const uint16_t mss);

    void init() {
      ISS = Connection::generate_iss();
      recover = ISS; // [RFC 6582]
    }

    uint32_t get_ts_recent() const noexcept { return TS_recent; }

    bool slow_start() const noexcept
    { return cwnd < ssthresh; }

    std::string to_string() const;
  }__attribute__((packed)); // < struct Connection::TCB

  /**
   * @brief      Creates a connection with a remote end point
   *
   * @param      host        The TCP host
   * @param[in]  local_port  The local port
   * @param[in]  remote      The remote socket
   * @param[in]  callback    The connection callback
   */
  Connection(TCP& host, Socket local, Socket remote, ConnectCallback callback = nullptr);
  ~Connection();

  Connection(const Connection&)             = delete;
  Connection(Connection&&)                  = delete;
  Connection& operator=(const Connection&)  = delete;
  Connection& operator=(Connection&&)       = delete;

  /**
   * @brief      Open the connection.
   *             Active determines whether the connection is active or passive.
   *             May throw if no remote host, or state isnt valid for opening.
   *
   * @param[in]  active  Whether its an active (outgoing) or passive (listening)
   */
  void open(bool active = false);

  /**
   * @brief      Set remote Socket bound to this connection.
   *
   * @param[in]  remote  The remote socket
   */
  void set_remote(Socket remote)
  { remote_ = remote; }

  // ???
  void deserialize_from(void*);
  int  serialize_to(void*) const;
  static const int VERSION = 2;

  /**
   * @brief      Reset all callbacks back to default
   */
  void reset_callbacks();

  using Recv_window_getter = delegate<uint32_t()>;
  void set_recv_wnd_getter(Recv_window_getter func)
  { recv_wnd_getter = func; }

  void release_memory() {
    read_request = nullptr;
    bufalloc.reset();
  }

private:
  /** "Parent" for Connection. */
  TCP& host_;

  /* End points. */
  Socket local_;
  Socket remote_;

  const bool is_ipv6_ = false;

  /** The current state the Connection is in. Handles most of the logic. */
  State* state_;
  // Previous state. Used to keep track of state transitions.
  State* prev_state_;

  /** Keep tracks of all sequence variables. */
  TCB cb;

  /** The given read request */
  std::unique_ptr<Read_request> read_request;
  os::mem::Pmr_pool::Resource_ptr bufalloc{nullptr};

  /** Queue for write requests to process */
  Write_queue writeq;

  /** Round Trip Time Measurer */
  RTTM rttm;

  /** Callbacks */
  ConnectCallback         on_connect_;
  DisconnectCallback      on_disconnect_;
  CloseCallback           on_close_;

  /** Retransmission timer */
  Timer rtx_timer;

  /** Time Wait / DACK timeout timer */
  Timer timewait_dack_timer;

  Recv_window_getter recv_wnd_getter;

  seq_t fin_seq_ = 0;
  bool fin_recv_ = false;

  bool close_signaled_ = false;

  /** Number of retransmission attempts on the packet first in RT-queue */
  int8_t rtx_attempt_ = 0;

  /** number of retransmitted SYN packets. */
  int8_t syn_rtx_ = 0;

  /** State if connection is in TCP write queue or not. */
  bool queued_;

  using Sack_list = sack::List<sack::Fixed_list<default_sack_entries>>;
  std::unique_ptr<Sack_list> sack_list;
  /** If SACK is permitted (option has been seen from peer) */
  bool sack_perm = false;
  size_t bytes_sacked_ = 0;

  /** Congestion control */
  // is fast recovery state
  bool fast_recovery_ = false;
  // First partial ack seen
  bool reno_fpack_seen = false;
  /** limited transmit [RFC 3042] active */
  bool limited_tx_ = true;
  // Number of current duplicate ACKs. Is reset for every new ACK.
  uint16_t dup_acks_ = 0;

  seq_t highest_ack_ = 0;
  seq_t prev_highest_ack_ = 0;
  uint32_t last_acked_ts_ = 0;

  /** Delayed ACK - number of seg received without ACKing */
  uint8_t  dack_{0};
  seq_t    last_ack_sent_;

  /**
   *  The size of the largest segment that the sender can transmit
   *  Updated by the Path MTU Discovery process (RFC 1191)
   */
  uint16_t smss_;

  /** RFC 3522 - The Eifel Detection Algorithm for TCP */
  //int16_t spurious_recovery = 0;
  //static constexpr int8_t SPUR_TO {1};
  //uint32_t rtx_ts_ = 0;
  /** RFC 4015 - The Eifel Response Algorithm for TCP */
  //uint32_t pipe_prev = 0;
  //static constexpr int8_t LATE_SPUR_TO {1};
  //RTTM::seconds SRTT_prev{1.0f};
  //RTTM::seconds RTTVAR_prev{1.0f};

  /**
   * @brief      Set the Read_request
   *
   * @param[in]  recv_bufsz  The receive bufsz
   * @param[in]  cb          The read callback
   */
  void _on_read(size_t recv_bufsz, ReadCallback cb);

  /**
   * @brief      Set the on_data handler
   *
   * @param[in]  cb          The callback
   */
  void _on_data(DataCallback cb);


  // Retrieve the associated shared_ptr for a connection, if it exists
  // Throws out_of_range if it doesn't
  Connection_ptr retrieve_shared();

  /// --- CALLBACKS --- ///

  /**
   * @brief Cleanup callback
   * @details This is called to make sure TCP/Listener doesn't hold any shared ptr of
   * the given connection. This is only for internal use, and not visible for the user.
   *
   * @param  Connection to be cleaned up
   */
  using CleanupCallback   = delegate<void(const Connection* self)>;
  CleanupCallback         _on_cleanup_;
  inline Connection&      _on_cleanup(CleanupCallback cb);

  void default_on_disconnect(Connection_ptr, Disconnect);

  /// --- READING --- ///

  /*
    Receive data into the current read requests buffer.
  */
  size_t receive(seq_t seq, const uint8_t* data, size_t n, bool PUSH);

  /*
    Remote is closing, no more data will be received.
    Returns receive buffer to user.
  */
  void receive_disconnect();

  void update_fin(const Packet_view& pkt);

  bool should_handle_fin() const noexcept
  { return fin_recv_ and static_cast<int32_t>(fin_seq_ - cb.RCV.NXT) == 0; }

  void handle_fin();

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
    Mark whether the Connection is in TCP write queue or not.
  */
  void set_queued(bool queued)
  { queued_ = queued; }

  /**
   * @brief      Sets the size of the largest segment that the sender can transmit
   *             Updated through the Path MTU Discovery process (RFC 1191) when TCP
   *             gets a notification about an updated PMTU value for this connection
   *             Set in TCP if Path MTU Discovery is enabled
   *
   * @param[in]  smss  The new SMSS (The PMTU value minus the size of the IP4 header and
   *                   the size of the TCP header)
   */
  void set_SMSS(uint16_t smss) noexcept
  { smss_ = smss; }

  /**
   * @brief      Sends an acknowledgement.
   */
  void send_ack();

  /*
    Invoke/signal the diffrent TCP events.
  */
  void signal_connect(const bool success = true);

  void signal_disconnect(Disconnect::Reason&& reason)
  { on_disconnect_(retrieve_shared(), Disconnect{reason}); }

  void signal_rtx_timeout()
  { }

  /*
    Drop a packet. Used for debug/callback.
  */
  void drop(const Packet_view& packet, Drop_reason reason = Drop_reason::NA);

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
  uint32_t usable_window() const noexcept
  {
    const int64_t x = (int64_t)send_window() - (int64_t)flight_size();
    return (uint32_t) std::max(static_cast<int64_t>(0), x);
  }

  uint32_t send_window() const noexcept
  { return std::min(cb.SND.WND, cb.cwnd); }

  uint32_t flight_size() const noexcept
  { return cb.SND.NXT - cb.SND.UNA; }

  bool uses_window_scaling() const noexcept;

  bool uses_timestamps() const noexcept;

  bool uses_SACK() const noexcept;

  /// --- INCOMING / TRANSMISSION --- ///
  /*
    Receive a TCP Packet.
  */
  void segment_arrived(Packet_view&);

  /*
    Acknowledge a packet
    - TCB update, Congestion control handling, RTT calculation and RT handling.
  */
  bool handle_ack(const Packet_view&);

  void update_rcv_wnd() {
    cb.RCV.WND = (recv_wnd_getter == nullptr) ?
      calculate_rcv_wnd() : recv_wnd_getter();
  }

  uint32_t calculate_rcv_wnd() const;

  void send_window_update() {
    update_rcv_wnd();
    send_ack();
  }

  void trigger_window_update(os::mem::Pmr_resource& res);

  /**
   * @brief      Receive data from an incoming packet containing data.
   *
   * @param[in]  in  TCP Packet containing payload
   */
  void recv_data(const Packet_view& in);

  void recv_out_of_order(const Packet_view& in);

  /**
   * @brief      Acknowledge incoming data. This is done by:
   *             - Trying to send data if possible (can send)
   *             - If not, regular ACK (use DACK if enabled)
   */
  void ack_data();

  /**
   * @brief      Determines if the incoming segment is a legit window update.
   *
   * @param[in]  in    TCP Segment
   * @param[in]  win   The calculated window
   *
   * @return     True if window update, False otherwise.
   */
  bool is_win_update(const Packet_view& in, const uint32_t win) const
  {
    return cb.SND.WND != win and
      (cb.SND.WL1 < in.seq() or (cb.SND.WL1 == in.seq() and cb.SND.WL2 <= in.ack()));
  }

  /**
   * @brief      Determines if duplicate acknowledge, described in [RFC 5681] p.3
   *
   * @param[in]  in    TCP segment
   *
   * @return     True if duplicate acknowledge, False otherwise.
   */
  bool is_dup_ack(const Packet_view& in, const uint32_t win) const
  {
    return in.ack() == cb.SND.UNA
      and flight_size() > 0
      and !in.has_tcp_data()
      and cb.SND.WND == win
      and !in.isset(SYN) and !in.isset(FIN);
  }

  /**
   * @brief      Handle duplicate ACK according to New Reno
   *
   * @param[in]  <unnamed>  Incoming TCP segment (duplicate ACK)
   */
  void on_dup_ack(const Packet_view&);

  /**
   * @brief      Handle segment according to congestion control (New Reno)
   *
   * @param[in]  <unnamed>  Incoming TCP segment
   */
  void congestion_control(const Packet_view&);

  /**
   * @brief      Handle segment according to fast recovery (New Reno)
   *
   * @param[in]  <unnamed>  Incoming TCP segment
   */
  void fast_recovery(const Packet_view&);

  /**
   * @brief      Determines ability to send ONE segment, not caring about the usable window.
   *
   * @return     True if able to send one, False otherwise.
   */
  bool can_send_one() const
  { return send_window() >= SMSS() and writeq.has_remaining_requests(); }

  /**
   * @brief      Send as much as possible from write queue.
   */
  void send_much()
  { writeq_push(); }

  /**
   * @brief      Fills the packet with data, limited to SMSS
   *
   * @param      packet  The packet
   * @param[in]  data    The data
   * @param[in]  n       The number of bytes to fill
   *
   * @return     The amount of data filled into the packet.
   */
  size_t fill_packet(Packet_view& packet, const uint8_t* data, size_t n)
  { return packet.fill(data, std::min(n, (size_t)SMSS())); }

  /*
    Transmit the packet and hooks up retransmission.
  */
  void transmit(Packet_view_ptr);

  /*
    Creates a new outgoing packet with the current TCB values and options.
  */
  Packet_view_ptr create_outgoing_packet();

  Packet_view_ptr outgoing_packet()
  { return create_outgoing_packet(); }

  uint16_t MSS() const noexcept;

  /**
   * @brief      Maximum Segment Data Size
   *             Limits the size for outgoing packets
   *
   * @return     MSDS
   */
  uint16_t MSDS() const noexcept;


  /// --- Congestion Control [RFC 5681] --- ///

  void setup_congestion_control()
  { reno_init(); }

  /**
   * @brief      Sender Maximum Segment Size
   *             The size of the largest segment that the sender can transmit
   *
   * @return     SMSS
   */
  uint16_t SMSS() const noexcept {
    return smss_; // Updated by Path MTU Discovery process
  }

  /**
   * @brief      Receiver Maximum Segment Size
   *             The size of the largest segment the receiver is willing to accept
   *
   * @return     RMSS
   */
  uint16_t RMSS() const noexcept
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
  { return static_cast<int32_t>(ACK - cb.recover) > 1; }



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

  /**
   * @brief      Take an RTT measurment from an incoming packet.
   *             Uses timestamp if timestamp options are in use,
   *             else RTTM start/stop.
   *
   * @param[in]  <unnamed>  An incomming TCP packet
   */
  void take_rtt_measure(const Packet_view&);

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
  { return rtx_attempt_ >= 14 or syn_rtx_ >= 4; };

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

  /** Whether to use Delayed ACK or not */
  bool use_dack() const noexcept;

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
  void parse_options(const Packet_view&);

  /*
    Add an option.
  */
  void add_option(Option::Kind, Packet_view&);

  void add_syn_options(Packet_view& pkt);
  void add_synack_options(Packet_view& pkt);


}; // < class Connection

} // < namespace tcp
} // < namespace net

#include "connection.inc"

#endif // < NET_TCP_CONNECTION_HPP
