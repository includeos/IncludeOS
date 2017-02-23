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
#ifndef NET_TCP_HPP
#define NET_TCP_HPP

#include <map>
#include <net/inet.hpp>
#include <net/util.hpp> // net::Packet_ptr
#include <sstream> // ostringstream
#include <queue> // buffer

#include "common.hpp"
#include "connection.hpp"
#include "headers.hpp"
#include "listener.hpp"
#include "socket.hpp"


namespace net {

  class TCP {
  public:
    using IPStack         = IP4::Stack;

    using CleanupCallback = tcp::Connection::CleanupCallback;
    using ConnectCallback = tcp::Connection::ConnectCallback;

    friend class tcp::Connection;
    friend class tcp::Listener;

  private:
    using Listeners       = std::map<tcp::port_t, std::unique_ptr<tcp::Listener>>;
    using Connections     = std::map<tcp::Connection::Tuple, tcp::Connection_ptr>;

  public:
    /////// TCP Stuff - Relevant to the protocol /////

    /// USER INTERFACE - TCP ///

    /*
      Constructor
    */
    TCP(IPStack&);

    /*
      Bind a new listener to a given Port.
    */
    tcp::Listener& bind(const tcp::port_t port, ConnectCallback cb = nullptr);

    /**
     * @brief Unbind (and close) a Listener
     * @details Closes the Listener and removes it from the
     * map of listeners
     *
     * @param port listening port
     * @return wether the listener had a port
     */
    bool unbind(const tcp::port_t port);

    /*
      Active open a new connection to the given remote.
    */
    void connect(tcp::Socket remote, ConnectCallback);

    void connect(tcp::Address address, tcp::port_t port, ConnectCallback callback)
    { connect({address, port}, std::move(callback)); }

    /*
      Active open a new connection to the given remote.
    */
    tcp::Connection_ptr connect(tcp::Socket remote);

    auto connect(tcp::Address address, tcp::port_t port)
    { return connect({address, port}); }

    /*
     * Insert existing connection
     */
    void insert_connection(tcp::Connection_ptr);

    /*
      Receive packet from network layer (IP).
    */
    void bottom(net::Packet_ptr);

    /*
      Delegate output to network layer
    */
    void set_network_out(downstream del)
    { _network_layer_out = del; }

    /*
      Compute the TCP checksum
    */
    static uint16_t checksum(const tcp::Packet&);

    const auto& listeners()
    { return listeners_; }

    const auto& connections()
    { return connections_; }

    /*
      Number of open ports.
    */
    size_t open_ports()
    { return listeners_.size(); }

    /*
      Number of active connections.
    */
    size_t active_connections()
    { return connections_.size(); }

    /*
      Set Maximum Segment Lifetime
    */
    void set_MSL(const std::chrono::milliseconds msl)
    { max_seg_lifetime_ = msl; }

    /*
      Maximum Segment Lifetime
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
    constexpr uint32_t window_size() const
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
    constexpr uint8_t wscale() const
    { return wscale_; }

    /**
     * @brief      Returns wether this TCP is using window scaling.
     *             A wscale factor of 0 means off.
     *
     * @return     Wether wscale is being used
     */
    constexpr bool uses_wscale() const
    { return wscale_ > 0; }

    void set_timestamps(bool active)
    { timestamps_ = active; }

    constexpr bool uses_timestamps() const
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


    void set_max_syn_backlog(const uint16_t limit)
    { max_syn_backlog_ = limit; }

    constexpr uint16_t max_syn_backlog() const
    { return max_syn_backlog_; }

    /*
      Maximum Segment Size
      [RFC 793] [RFC 879] [RFC 6691]
    */
    constexpr uint16_t MSS() const
    { return network().MDDS() - sizeof(tcp::Header); }

    /*
      Show all connections for TCP as a string.
    */
    std::string to_string() const;

    std::string status() const
    { return to_string(); }

    size_t writeq_size() const
    { return writeq.size(); }

    tcp::Address address()
    { return inet_.ip_addr(); }

    IPStack& stack() const
    { return inet_; }

  private:
    IPStack&    inet_;
    Listeners   listeners_;
    Connections connections_;

    downstream  _network_layer_out;

    /** Internal writeq - connections gets queued in the wait for packets and recvs offer */
    std::deque<tcp::Connection_ptr> writeq;

    /** Current outgoing port */
    tcp::port_t current_ephemeral_;

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

    /*
      Transmit packet to network layer (IP).
    */
    void transmit(tcp::Packet_ptr);

    /*
      Generate a unique initial sequence number (ISS).
    */
    static tcp::seq_t generate_iss();

    /*
      Returns a free port for outgoing connections.
    */
    tcp::port_t next_free_port();

    /*
      Check if the port is in use either among "listeners" or "connections"
    */
    bool port_in_use(const tcp::port_t) const;

    uint32_t get_ts_value() const;

    /*
      Packet is dropped.
    */
    void drop(const tcp::Packet&);

    /*
      Add a Connection.
    */
    tcp::Connection_ptr add_connection(tcp::port_t local_port,
                                       tcp::Socket remote,
                                       ConnectCallback cb = nullptr);

    void add_connection(tcp::Connection_ptr);

    /*
      Close and delete the connection.
    */
    void close_connection(tcp::Connection_ptr);

    void close_listener(tcp::Listener&);


    /*
      Process the write queue with the given amount of free packets.
    */
    void process_writeq(size_t packets);

    /*

    */
    void request_offer(tcp::Connection&);

    void queue_offer(tcp::Connection_ptr);

    /*
      Force the TCP to process the it's queue with the current amount of available packets.
    */
    void kick()
    { process_writeq(inet_.transmit_queue_available()); }

    IP4& network() const
    { return inet_.ip_obj(); }

  }; // < class TCP

} // < namespace net

#endif // < NET_TCP_HPP
