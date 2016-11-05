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

#include <chrono> // timer duration
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
    tcp::Listener& bind(const tcp::port_t port);

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
    tcp::Connection_ptr connect(tcp::Socket remote);

    /*
      Active open a new connection to the given remote.
    */
    void connect(tcp::Socket remote, ConnectCallback);

    auto connect(tcp::Address address, tcp::port_t port)
    { return connect({address, port}); }

    void connect(tcp::Address address, tcp::port_t port, ConnectCallback callback)
    { connect({address, port}, callback); }

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
      Maximum Segment Lifetime
    */
    auto MSL() const
    { return MAX_SEG_LIFETIME; }

    /*
      Set Maximum Segment Lifetime
    */
    void set_MSL(const std::chrono::milliseconds msl)
    { MAX_SEG_LIFETIME = msl; }

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

    /** Stats */
    uint64_t& bytes_rx_;
    uint64_t& bytes_tx_;
    uint64_t& packets_rx_;
    uint64_t& packets_tx_;
    uint64_t& incoming_connections_;
    uint64_t& outgoing_connections_;
    uint64_t& connection_attempts_;
    uint32_t& packets_dropped_;

    IPStack& inet_;
    using Listeners = std::map<tcp::port_t, std::unique_ptr<tcp::Listener>>;
    using Connections = std::map<tcp::Connection::Tuple, tcp::Connection_ptr>;
    Listeners listeners_;
    Connections connections_;

    downstream _network_layer_out;

    std::deque<tcp::Connection_ptr> writeq;

    /*
      Settings
    */
    tcp::port_t current_ephemeral_ = 1024;

    std::chrono::milliseconds MAX_SEG_LIFETIME;

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

    /*
      Packet is dropped.
    */
    void drop(const tcp::Packet&);

    /*
      Add a Connection.
    */
    tcp::Connection_ptr add_connection(tcp::port_t local_port, tcp::Socket remote);

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
    size_t send(tcp::Connection_ptr, const char* buffer, size_t n);

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
