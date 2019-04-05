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
#ifndef NET_TCP_HPP
#define NET_TCP_HPP

#include "common.hpp"
#include "connection.hpp"
#include "headers.hpp"
#include "listener.hpp"
#include "packet_view.hpp"
#include "packet.hpp" // remove me, temp for NaCl

#include <map>  // connections, listeners
#include <deque>  // writeq
#include <net/socket.hpp>
#include <net/ip4/ip4.hpp>
#include <util/bitops.hpp>
#include <util/alloc_pmr.hpp>

namespace net {

  class Inet;

  struct TCP_error : public std::runtime_error {
    using runtime_error::runtime_error;
  };
  /**
   * @brief      An access point for TCP, handling connections, config and such.
   */
  class TCP {
  public:
    using IPStack         = IP4::Stack;

    using CleanupCallback = tcp::Connection::CleanupCallback;
    using ConnectCallback = tcp::Connection::ConnectCallback;

    using Packet_reroute_func = delegate<void(net::Packet_ptr)>;

    using Port_utils = std::map<net::Addr, Port_util>;

    friend class tcp::Connection;
    friend class tcp::Listener;

  private:
    using Listeners       = std::map<Socket, std::shared_ptr<tcp::Listener>>;
    using Connections     = std::unordered_map<tcp::Connection::Tuple, tcp::Connection_ptr>;

  public:
    /////// TCP Stuff - Relevant to the protocol /////

    /// USER INTERFACE - TCP ///

    /**
     * @brief      Construct a TCP instance for a given IPStack with default values.
     *
     * @param      <unnamed>  The IPStack used by TCP
     */
    TCP(IPStack&, bool smp = false);

    /**
     * @brief      Bind to a port to start listening for new connections
     *             Throws if unable to bind
     *
     * @param[in]  port  The port
     * @param[in]  cb    (optional) Connect callback to be invoked on new connections.
     *
     * @return     A TCP Listener
     */
    tcp::Listener& listen(const tcp::port_t port, ConnectCallback cb = nullptr,
                          const bool ipv6_only = false);

    /**
     * @brief      Bind to a socket to start listening for new connections
     *             Throws if unable to bind
     *
     * @param[in]  socket  The socket
     * @param[in]  cb      (optional) Connect callback to be invoked on new connections.
     *
     * @return     A TCP Listener
     */
    tcp::Listener& listen(const Socket& socket, ConnectCallback cb = nullptr);

    /**
     * @brief Close a Listener
     * @details Closes the Listener and removes it from the
     * map of listeners
     *
     * @param socket listening socket
     * @return whether the listener existed and was closed
     */
    bool close(const Socket& socket);

    /**
     * @brief      Make an outgoing connection to a TCP remote (IP:port).
     *             May throw if no available ephemeral ports.
     *
     * @param[in]  remote     The remote
     * @param[in]  cb         Connect callback to be invoked when the connection is established.
     */
    void connect(Socket remote, ConnectCallback cb);

    /**
     * @brief      Make an outgoing connection from a given source address.
     *             May throw if the source address can not be bound to, or if
     *             no available ephemeral port.
     *
     * @param[in]  source    The source address
     * @param[in]  remote    The remote socket
     * @param[in]  callback  The connect callback
     */
    void connect(tcp::Address source, Socket remote, ConnectCallback callback);

    /**
     * @brief      Make an outgoing connection to from a given source socket.
     *             May throw if the socket cannot be bound to (for different reasons).
     *
     * @param[in]  local     The local socket
     * @param[in]  remote    The remote socket
     * @param[in]  callback  The connect callback
     */
    void connect(Socket local, Socket remote, ConnectCallback callback);

    /**
     * @brief      Make an outgoing connecction to a TCP remote (IP:port).
     *             May throw if no available ephemeral ports.
     *
     * @param[in]  remote  The remote socket
     *
     * @return     A ptr to an unestablished TCP Connection
     */
    tcp::Connection_ptr connect(Socket remote);

    /**
     * @brief      Make an outgoing connection from a given source address.
     *             May throw if the source address can not be bound to, or if
     *             no available ephemeral port.
     *
     * @param[in]  source  The source
     * @param[in]  remote  The remote
     *
     * @return     A ptr to an unestablished TCP Connection
     */
    tcp::Connection_ptr connect(tcp::Address source, Socket remote);

    /**
     * @brief      Make an outgoing connection to from a given source socket.
     *             May throw if the socket cannot be bound to (for different reasons).
     *
     * @param[in]  local   The local
     * @param[in]  remote  The remote
     *
     * @return     A ptr to an unestablished TCP Connection
     */
    tcp::Connection_ptr connect(Socket local, Socket remote);

    /**
     * @brief      Insert a connection ptr into the TCP (used for restoring)
     *
     * @param[in]  <unnamed>  A ptr to an TCP Connection
     */
    void insert_connection(tcp::Connection_ptr);

    /**
     * @brief      Receive a Packet from the network layer (IP)
     *
     * @param[in]  <unnamed>  A network packet
     */
    void receive4(net::Packet_ptr);

    /**
     * @brief      Receive a Packet from the network layer (IP6)
     *
     * @param[in]  <unnamed>  A IP6 packet
     */
    void receive6(net::Packet_ptr);

    void receive(tcp::Packet_view&);

    /**
     * @brief      Sets a delegate to the network output.
     *
     * @param[in]  del   A downstream delegate
     */
    void set_network_out4(downstream del)
    { network_layer_out4_ = del; }

    void set_network_out6(downstream del)
    { network_layer_out6_ = del; }

    /**
     * @brief      Returns a collection of the listeners for this instance.
     *
     * @return     A collection of Listeners
     */
    const auto& listeners() const
    { return listeners_; }

    /**
     * @brief      Returns a collection of the connections for this instance.
     *
     * @return     A collection of Connections
     */
    const auto& connections() const
    { return connections_; }

    /**
     * @brief      Number of listening ports
     *
     * @return     Number of listening ports
     */
    size_t listening_ports() const
    { return listeners_.size(); }

    /**
     * @brief      The total number of active connections
     *
     * @return     The total number of active connections
     */
    size_t active_connections() const
    { return connections_.size(); }

    /**
     * @brief      Sets the MSL (Maximum Segment Lifetime)
     *
     * @param[in]  msl   The MSL in milliseconds
     */
    void set_MSL(const std::chrono::milliseconds msl)
    { max_seg_lifetime_ = msl; }

    /**
     * @brief      The MSL (Maximum Segment Lifetime)
     *
     * @return     The MSL in milliseconds
     */
    auto MSL() const
    { return max_seg_lifetime_; }

    /**
     * @brief      Sets the window size advertisted per Connection.
     *
     * @param[in]  wsize  The window size
     */
    void set_window_size(const uint32_t wsize)
    { Expects(wsize <= 0x40000000); win_size_ = wsize; }

    /**
     * @brief      Sets the window size with a windowscale factor.
     *             The wsize is left shifted with the factor supplied:
     *             wsize << factor (wsize * 2^factor)
     *
     * @param[in]  wsize   The window size
     * @param[in]  factor  The wscale factor
     */
    void set_window_size(const uint32_t wsize, const uint8_t factor)
    {
      set_wscale(factor);
      set_window_size(wsize << factor);
    }

    /**
     * @brief      Returns the window size set.
     *
     * @return     The window size
     */
    uint32_t window_size() const
    { return win_size_; }

    /**
     * @brief      Sets the window scale factor [RFC 7323] p. 8
     *             Setting factor to 0 means wscale is turned off.
     *
     * @param[in]  factor  The wscale factor
     */
    void set_wscale(const uint8_t factor)
    { Expects(factor <= 14 && "WScale factor cannot exceed 14"); wscale_ = factor; }

    /**
     * @brief      Returns the current wscale factor set.
     *
     * @return     The wscale factor
     */
    uint8_t wscale() const
    { return wscale_; }

    /**
     * @brief      Returns whether this TCP is using window scaling.
     *             A wscale factor of 0 means off.
     *
     * @return     Whether wscale is being used
     */
    bool uses_wscale() const
    { return wscale_ > 0; }

    /**
     * @brief      Sets if Timestamp Options is gonna be used.
     *
     * @param[in]  active  Whether Timestamp Options are in use.
     */
    void set_timestamps(bool active)
    { timestamps_ = active; }

    /**
     * @brief      Whether the TCP instance is using Timestamp Options or not.
     *
     * @return     Whether the TCP instance is using Timestamp Options or not
     */
    bool uses_timestamps() const
    { return timestamps_; }

    /**
     * @brief      Sets if SACK Option is gonna be used.
     *
     * @param[in]  active  Whether SACK Option are in use.
     */
    void set_SACK(bool active) noexcept
    { sack_ = active; }

    /**
     * @brief      Whether the TCP instance is using SACK Options or not.
     *
     * @return     Whether the TCP instance is using SACK Options or not.
     */
    bool uses_SACK() const noexcept
    { return sack_; }

    /**
     * @brief      Sets the dack. [RFC 1122] (p.96)
     *
     * @param[in]  dack_timeout  The dack timeout
     */
    void set_DACK(const std::chrono::milliseconds dack_timeout)
    { dack_timeout_ = dack_timeout; }

    /**
     * @brief      Returns the delayed ACK timeout (ms)
     *
     * @return     time to wait before sending an ACK
     */
    auto DACK_timeout() const
    { return dack_timeout_; }

    /**
     * @brief      Sets the maximum amount of allowed concurrent connection attempts.
     *
     * @param[in]  limit  The limit
     */
    void set_max_syn_backlog(const uint16_t limit)
    { max_syn_backlog_ = limit; }

    /**
     * @brief      The maximum amount of allowed concurrent connection attempts.
     *
     * @return     The limit
     */
    uint16_t max_syn_backlog() const
    { return max_syn_backlog_; }

    /**
     * @brief      Set the maximum allowed memory
     *             to be used by this TCP.
     *
     * @param[in]  size  The limit in bytes
     */
    void set_total_bufsize(const size_t size)
    {
      total_bufsize_ = size;
      mempool_.set_total_capacity(total_bufsize_);
    }

    const os::mem::Pmr_pool& mempool() {
      return mempool_;
    }

    /**
     * @brief      Sets the minimum buffer size.
     *
     * @param[in]  size  The size
     */
    void set_min_bufsize(const size_t size)
    {
      Expects(util::bits::is_pow2(size));
      Expects(size <= max_bufsize_);
      min_bufsize_ = size;
    }

    /**
     * @brief      Sets the maximum buffer size.
     *
     * @param[in]  size  The size
     */
    void set_max_bufsize(const size_t size)
    {
      Expects(util::bits::is_pow2(size));
      Expects(size >= min_bufsize_);
      max_bufsize_ = size;
    }

    auto min_bufsize() const
    { return min_bufsize_; }

    auto max_bufsize() const
    { return max_bufsize_; }

    /**
     * @brief      The Maximum Segment Size to be used for this instance.
     *             [RFC 793] [RFC 879] [RFC 6691]
     *             This is the MTU - IP_hdr - TCP_hdr
     *
     * @return     The MSS
     */
    uint16_t MSS(const Protocol) const;

    /**
     * @brief      Returns a string representation of the listeners and connections.
     *
     * @return     String representation of this instance.
     */
    std::string to_string() const;

    /**
     * @brief      Returns the status about the instance as a string.
     *
     * @return     An info string
     */
    std::string status() const
    { return to_string(); }

    /**
     * @brief      Determines if the socket is bound.
     *
     * @param[in]  socket  The socket
     *
     * @return     True if bound, False otherwise.
     */
    bool is_bound(const Socket& socket) const;

    /**
     * @brief      Number of connections queued for writing.
     *
     * @return     Number of connections queued for writing
     */
    size_t writeq_size() const
    { return writeq.size(); }

    /**
     * @brief      The IP address for which the TCP instance is "connected".
     *
     * @return     An IP address
     */
    tcp::Address address() const noexcept;

    /**
     * @brief      The stack object for which the TCP instance is "bound to"
     *
     * @return     An IP Stack obj
     */
    IPStack& stack() const
    { return inet_; }

    /**
     *  Methods for handling Path MTU Discovery - RFC 1191
     */

    /**
     * @brief      Is called when an Error has occurred in the OS
     *             F.ex.: Is called when an ICMP error message has been received in response
     *             to a sent TCP (or UDP) packet
     *
     * @param[in]  err   The error
     * @param[in]  dest  The destination the original packet was sent to, that resulted in an error
     */
    void error_report(const Error& err, Socket dest);

    /**
     * @brief      Called by Inet (triggered by IP) when a Path MTU value has grown stale and the value
     *             is reset (increased) to check if the PMTU for the path could have increased
     *             This is NOT a change in the Path MTU in response to receiving an ICMP Too Big message
     *             and no retransmission of packets should take place
     *
     * @param[in]  dest  The destination/path
     * @param[in]  pmtu  The reset PMTU value
     */
    void reset_pmtu(Socket dest, IP4::PMTU pmtu);

    /**
     * Return the associated shared_ptr for a connection, if it exists
     * Throws out_of_range if it doesn't
    **/
    tcp::Connection_ptr retrieve_shared(tcp::Connection* self)
    {
      auto i = connections_.find(self->tuple());
      if (i != connections_.end())
      {
        //printf("Found connection: %p\n", i->second.get());
        return i->second;
      }

      auto j = find_listener(self->local());
      if (j != listeners_.end())
      {
        //printf("Found listener\n");
        auto& q = j->second->syn_queue_;
        for (auto& conn : q) {
          if (conn.get() == self) {
            //printf("Found connection: %p\n", conn.get());
            return conn;
          }
        }
      }
      throw std::out_of_range("Missing connection");
    }

    void redirect(Packet_reroute_func func) {
      this->packet_rerouter = func;
    }

    int get_cpuid() const noexcept {
      return this->cpu_id;
    }

  private:
    IPStack&      inet_;
    Listeners     listeners_;
    Connections   connections_;

    size_t total_bufsize_;
    os::mem::Pmr_pool mempool_;

    size_t min_bufsize_;
    size_t max_bufsize_;

    Port_utils& ports_;

    downstream  network_layer_out4_;
    downstream  network_layer_out6_;

    /** Internal writeq - connections gets queued in the wait for packets and recvs offer */
    std::deque<tcp::Connection_ptr> writeq;

    /* Settings */

    /** Maximum segment lifetime (this affects TIME-WAIT period - a connection will be closed after 2*MSL) */
    std::chrono::milliseconds max_seg_lifetime_;
    /** The window size of the packet */
    uint32_t                  win_size_;
    /** Window scale factor [RFC 7323] p. 8 */
    uint8_t                   wscale_;
    /** Timestamp option active [RFC 7323] p. 11 */
    bool                      timestamps_;
    /** Selective ACK  [RFC 2018] */
    bool                      sack_;
    /** Delayed ACK timeout - how long should we wait with sending an ACK */
    std::chrono::milliseconds dack_timeout_;
    /** Maximum SYN queue backlog */
    uint16_t                  max_syn_backlog_;

    /** Stats */
    uint64_t* bytes_rx_ = nullptr;
    uint64_t* bytes_tx_ = nullptr;
    uint64_t* packets_rx_ = nullptr;
    uint64_t* packets_tx_ = nullptr;
    uint64_t* incoming_connections_ = nullptr;
    uint64_t* outgoing_connections_ = nullptr;
    uint64_t* connection_attempts_ = nullptr;
    uint32_t* packets_dropped_ = nullptr;

    bool smp_enabled = false;
    int  cpu_id = 0;
    Packet_reroute_func packet_rerouter = nullptr;

    /**
     * @brief      Transmit an outgoing TCP segment to the network.
     *             Makes sure the segment is okay, setting checksum and such.
     *
     * @param[in]  <unnamed>  A TCP Segment
     */
    void transmit(tcp::Packet_view_ptr);

    /**
     * @brief      Creates an outgoing TCP packet.
     *
     * @return     A tcp packet ptr
     */
    tcp::Packet_view_ptr create_outgoing_packet();

    /**
     * @brief      Creates an outgoing TCP6 packet.
     *
     * @return     A tcp packet ptr
     */
    tcp::Packet_view_ptr create_outgoing_packet6();

    /**
     * @brief      Sends a TCP reset based on the values of the incoming packet.
     *             Used when packet are addressed to closed ports or already dead connections.
     *
     * @param[in]  incoming  The incoming tcp packet "to reset".
     */
    void send_reset(const tcp::Packet_view& incoming);

    /**
     * @brief      Generate a unique initial sequence number (ISS).
     *
     * @return     A sequence number (SEQ)
     */
    static tcp::seq_t generate_iss();

    /**
     * @brief      Gets an incremental timestamp value.
     *
     * @return     The timestamp value.
     */
    uint32_t get_ts_value() const;

    /**
     * @brief      The IP4 object bound to the IPStack
     *
     * @return     An IP4 object
     */
    IP4& network() const;

    /**
     * @brief      Drops the TCP segment
     *
     * @param[in]  <unnamed>  A TCP Segment
     */
    void drop(const tcp::Packet_view&);


    // INTERNALS - Handling of collections

    /**
     * @brief      Binds a socket, reserving it for future use
     *             (until unbound)
     *             Throws if unable to bind.
     *
     * @param[in]  socket  The socket
     */
    void bind(const Socket& socket);

    /**
     * @brief      Unbinds a socket, making it free for future use
     *
     * @param[in]  socket  The socket
     *
     * @return     Returns wether there was a socket that got unbound
     */
    bool unbind(const Socket& socket);

    /**
     * @brief      Bind to an socket where the address is given and the
     *             port is an ephemeral port.
     *             Throws if there are no more free ephemeral ports.
     *
     * @param[in]  addr  The address
     *
     * @return     The socket that got bound.
     */
    Socket bind(const tcp::Address& addr);

    /**
     * @brief      Determines if the source address is valid.
     *
     * @param[in]  addr  The source address
     *
     * @return     True if valid source, False otherwise.
     */
    bool is_valid_source(const tcp::Address& addr) const noexcept;

    /**
     * @brief      Try to find the listener bound to socket.
     *             If none is found directly, try any address (0).
     *
     * @param[in]  socket  The socket the listener is bound to
     *
     * @return     A listener iterator
     */
    Listeners::iterator find_listener(const Socket& socket);

    /**
     * @brief      Try to find the listener bound to socket.
     *             If none is found directly, try any address (0).
     *
     * @param[in]  socket  The socket the listener is bound to
     *
     * @return     A listener const iterator
     */
    Listeners::const_iterator cfind_listener(const Socket& socket) const;

    /**
     * @brief      Adds a connection.
     *
     * @param[in]  <unnamed>  A ptr to the Connection
     *
     * @return     True if the connection was added, false if rejected
     */
    bool add_connection(tcp::Connection_ptr);

    /**
     * @brief      Creates a connection.
     *
     * @param[in]  local    The local socket
     * @param[in]  remote   The remote socket
     * @param[in]  cb       Connect callback
     *
     * @return     A ptr to the Connection whos created
     */
    tcp::Connection_ptr create_connection(Socket local,
                                          Socket remote,
                                          ConnectCallback cb = nullptr);

    /**
     * @brief      Closes and deletes a connection.
     *
     * @param[in]  conn  A ptr to a Connection
     */
    void close_connection(const tcp::Connection* conn)
    {
      unbind(conn->local());
      connections_.erase(conn->tuple());
    }

    /**
     * @brief      Closes and deletes a listener.
     *
     * @param[in]  listener  A Listener
     */
    void close_listener(tcp::Listener& listener);


    // WRITEQ HANDLING

    /**
     * @brief      Process the write queue with the given amount of free packets.
     *
     * @param[in]  packets  Number of disposable packets
     */
    void process_writeq(size_t packets);
    void smp_process_writeq(size_t packets);

    /**
     * @brief      Request an offer of packets.
     *             Used when a Connection wants to write
     *
     * @param      <unnamed>  A ptr to a Connection
     */
    void request_offer(tcp::Connection&);

    /**
     * @brief      Queue offer for when packets are available.
     *             Used when a Connection wants an offer to write.
     *
     * @param[in]  <unnamed>  A ptr to a Connection
     */
    void queue_offer(tcp::Connection&);

    /**
     * @brief      Force the TCP to process the it's queue with the current amount of available packets.
     */
    void kick();

  }; // < class TCP

} // < namespace net

#endif // < NET_TCP_HPP
