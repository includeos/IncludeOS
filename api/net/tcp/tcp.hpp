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
#include "packet.hpp"

#include <map>  // connections, listeners
#include <queue>  // writeq
#include <net/inet.hpp>
#include <net/socket.hpp>
#include <bitset>

namespace net {

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

    friend class tcp::Connection;
    friend class tcp::Listener;

  private:
    using Listeners       = std::map<Socket, std::unique_ptr<tcp::Listener>>;
    using Connections     = std::map<tcp::Connection::Tuple, tcp::Connection_ptr>;

    /**
     * @brief      Class for port utility.
     */
    class Port_util {
    public:
      /**
       * @brief      Construct a port util with a new generated ephemeral port
       *             and a empty port list.
       */
      Port_util();

      /**
       * @brief      Gets the next ephemeral port.
       *             increment_ephemeral may throw
       *
       * @return     The next ephemeral port.
       */
      uint16_t get_next_ephemeral()
      {
        increment_ephemeral();
        return ephemeral_;
      }

      /**
       * @brief      Bind a port, making it reserved.
       *
       * @param[in]  port  The port
       */
      void bind(const uint16_t port) noexcept
      {
        Expects(port < port_ranges::DYNAMIC_END);
        ports.set(port);

        if(port_ranges::is_dynamic(port)) ++eph_count;
      }

      /**
       * @brief      Unbind a port, making it available.
       *
       * @param[in]  port  The port
       */
      void unbind(const uint16_t port) noexcept
      {
        Expects(port < port_ranges::DYNAMIC_END);
        ports.reset(port);

        if(port_ranges::is_dynamic(port)) --eph_count;
      }

      /**
       * @brief      Determines if the port is bound.
       *
       * @param[in]  port  The port
       *
       * @return     True if bound, False otherwise.
       */
      bool is_bound(const uint16_t port) const noexcept
      {
        Expects(port < port_ranges::DYNAMIC_END);
        return ports[port];
      }

      /**
       * @brief      Determines if it has any free ephemeral ports.
       *
       * @return     True if has free ephemeral, False otherwise.
       */
      bool has_free_ephemeral() const noexcept
      { return eph_count < (port_ranges::DYNAMIC_END - port_ranges::DYNAMIC_START); }

    private:
      std::bitset<65536> ports;
      uint16_t           ephemeral_;
      uint16_t           eph_count;

      /**
       * @brief      Increment the ephemeral port by one.
       *             Throws if there are no more free ephemeral ports available.
       */
      void increment_ephemeral();

    }; // < class Port_util
    using Port_lists      = std::map<tcp::Address, Port_util>;

  public:
    /////// TCP Stuff - Relevant to the protocol /////

    /// USER INTERFACE - TCP ///

    /**
     * @brief      Construct a TCP instance for a given IPStack with default values.
     *
     * @param      <unnamed>  The IPStack used by TCP
     */
    TCP(IPStack&);

    /**
     * @brief      Bind to a port to start listening for new connections
     *             Throws if unable to bind
     *
     * @param[in]  port  The port
     * @param[in]  cb    (optional) Connect callback to be invoked on new connections.
     *
     * @return     A TCP Listener
     */
    tcp::Listener& listen(const tcp::port_t port, ConnectCallback cb = nullptr)
    { return listen({0, port}, std::move(cb)); }

    /**
     * @brief      Bind to a socket to start listening for new connections
     *             Throws if unable to bind
     *
     * @param[in]  socket  The socket
     * @param[in]  cb      (optional) Connect callback to be invoked on new connections.
     *
     * @return     A TCP Listener
     */
    tcp::Listener& listen(Socket socket, ConnectCallback cb = nullptr);

    /**
     * @brief Close a Listener
     * @details Closes the Listener and removes it from the
     * map of listeners
     *
     * @param socket listening socket
     * @return whether the listener existed and was closed
     */
    bool close(Socket socket);

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
    void receive(net::Packet_ptr);

    /**
     * @brief      Sets a delegate to the network output.
     *
     * @param[in]  del   A downstream delegate
     */
    void set_network_out(downstream del)
    { _network_layer_out = del; }

    /**
     * @brief      Computes the TCP checksum of a segment
     *
     * @param[in]  <unnamed>  a TCP Segment
     *
     * @return     The checksum of the TCP segment
     */
    static uint16_t checksum(const tcp::Packet&);

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
     * @brief      The Maximum Segment Size to be used for this instance.
     *             [RFC 793] [RFC 879] [RFC 6691]
     *             This is the MTU - IP_hdr - TCP_hdr
     *
     * @return     The MSS
     */
    uint16_t MSS() const
    { return network().MDDS() - sizeof(tcp::Header); }

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
    bool is_bound(const Socket socket) const;

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
     * @return     An IP4 address
     */
    tcp::Address address() const noexcept
    { return inet_.ip_addr(); }

    /**
     * @brief      The stack object for which the TCP instance is "bound to"
     *
     * @return     An IP Stack obj
     */
    IPStack& stack() const
    { return inet_; }

    /**
     *  Is called when an Error has occurred in the OS
     *  F.ex.: Is called when an ICMP error message has been received in response to a sent TCP packet
    */
    void error_report(Error& err, Socket dest);

  private:
    IPStack&      inet_;
    Listeners     listeners_;
    Connections   connections_;

    Port_lists    ports_;

    downstream  _network_layer_out;

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
    /** Delayed ACK timeout - how long should we wait with sending an ACK */
    std::chrono::milliseconds dack_timeout_;
    /** Maximum SYN queue backlog */
    uint16_t                  max_syn_backlog_;

    /** Stats */
    uint64_t& bytes_rx_;
    uint64_t& bytes_tx_;
    uint64_t& packets_rx_;
    uint64_t& packets_tx_;
    uint64_t& incoming_connections_;
    uint64_t& outgoing_connections_;
    uint64_t& connection_attempts_;
    uint32_t& packets_dropped_;

    /**
     * @brief      Transmit an outgoing TCP segment to the network.
     *             Makes sure the segment is okay, setting checksum and such.
     *
     * @param[in]  <unnamed>  A TCP Segment
     */
    void transmit(tcp::Packet_ptr);

    /**
     * @brief      Creates an outgoing TCP packet.
     *
     * @return     A tcp packet ptr
     */
    tcp::Packet_ptr create_outgoing_packet();

    /**
     * @brief      Sends a TCP reset based on the values of the incoming packet.
     *             Used when packet are addressed to closed ports or already dead connections.
     *
     * @param[in]  incoming  The incoming tcp packet "to reset".
     */
    void send_reset(const tcp::Packet& incoming);

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
    IP4& network() const
    { return inet_.ip_obj(); }

    /**
     * @brief      Drops the TCP segment
     *
     * @param[in]  <unnamed>  A TCP Segment
     */
    void drop(const tcp::Packet&);


    // INTERNALS - Handling of collections

    /**
     * @brief      Binds a socket, reserving it for future use
     *             (until unbound)
     *             Throws if unable to bind.
     *
     * @param[in]  socket  The socket
     */
    void bind(const Socket socket);

    /**
     * @brief      Unbinds a socket, making it free for future use
     *
     * @param[in]  socket  The socket
     *
     * @return     Returns wether there was a socket that got unbound
     */
    bool unbind(const Socket socket);

    /**
     * @brief      Bind to an socket where the address is given and the
     *             port is an ephemeral port.
     *             Throws if there are no more free ephemeral ports.
     *
     * @param[in]  addr  The address
     *
     * @return     The socket that got bound.
     */
    Socket bind(const tcp::Address addr);

    /**
     * @brief      Binds to an socket where the address is given by the
     *             stack and port is ephemeral. See bind(addr)
     *
     * @return     The socket that got bound.
     */
    Socket bind()
    { return bind(address()); }

    /**
     * @brief      Determines if the source address is valid.
     *
     * @param[in]  addr  The source address
     *
     * @return     True if valid source, False otherwise.
     */
    bool is_valid_source(const tcp::Address addr) const noexcept
    { return addr == address() or addr == 0; /* temp */ }

    /**
     * @brief      Try to find the listener bound to socket.
     *             If none is found directly, try any address (0).
     *
     * @param[in]  socket  The socket the listener is bound to
     *
     * @return     A listener iterator
     */
    Listeners::iterator find_listener(const Socket socket)
    {
      Listeners::iterator it = listeners_.find(socket);
      if(it == listeners_.end() and socket.address() != 0)
        it = listeners_.find({0, socket.port()});
      return it;
    }

    /**
     * @brief      Try to find the listener bound to socket.
     *             If none is found directly, try any address (0).
     *
     * @param[in]  socket  The socket the listener is bound to
     *
     * @return     A listener const iterator
     */
    Listeners::const_iterator cfind_listener(const Socket socket) const
    {
      Listeners::const_iterator it = listeners_.find(socket);
      if(it == listeners_.cend() and socket.address() != 0)
        it = listeners_.find({0, socket.port()});
      return it;
    }

    /**
     * @brief      Adds a connection.
     *
     * @param[in]  <unnamed>  A ptr to the Connection
     */
    void add_connection(tcp::Connection_ptr);

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
    void close_connection(tcp::Connection_ptr conn)
    {
      unbind(conn->local());
      connections_.erase(conn->tuple());
    }

    /**
     * @brief      Closes and deletes a listener.
     *
     * @param[in]  listener  A Listener
     */
    void close_listener(tcp::Listener& listener)
    {
      const auto socket = listener.local();
      unbind(socket);
      listeners_.erase(socket);
    }


    // WRITEQ HANDLING

    /**
     * @brief      Process the write queue with the given amount of free packets.
     *
     * @param[in]  packets  Number of disposable packets
     */
    void process_writeq(size_t packets);

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
    void queue_offer(tcp::Connection_ptr);

    /**
     * @brief      Force the TCP to process the it's queue with the current amount of available packets.
     */
    void kick()
    { process_writeq(inet_.transmit_queue_available()); }

  }; // < class TCP

} // < namespace net

#endif // < NET_TCP_HPP
