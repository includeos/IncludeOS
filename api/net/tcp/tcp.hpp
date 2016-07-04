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
#ifndef NET_TCP_TCP_HPP
#define NET_TCP_TCP_HPP

#include <os>
#include <net/ip4/ip4.hpp> // IP4::Addr
#include <net/util.hpp> // net::Packet_ptr
#include <queue> // buffer
#include <map>
#include <sstream> // ostringstream
#include <chrono> // timer duration

#include "common.hpp"
#include "socket.hpp"
#include "connection.hpp"

namespace net {
namespace tcp {

  class TCP {
  public:
    using IPStack = Inet<LinkLayer,IP4>;

    friend class Connection;

  public:
    /////// TCP Stuff - Relevant to the protocol /////

    static constexpr uint16_t default_window_size = 0xffff;

    static constexpr uint16_t default_mss = 536;

    /*
      Flags (Control bits) in the TCP Header.
    */
    enum Flag {
      NS        = (1 << 8),     // Nounce (Experimental: see RFC 3540)
      CWR       = (1 << 7),     // Congestion Window Reduced
      ECE       = (1 << 6),     // ECN-Echo
      URG       = (1 << 5),     // Urgent
      ACK       = (1 << 4),     // Acknowledgement
      PSH       = (1 << 3),     // Push
      RST       = (1 << 2),     // Reset
      SYN       = (1 << 1),     // Syn(chronize)
      FIN       = 1,            // Fin(ish)
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
      port_t source_port;                    // Source port
      port_t destination_port;               // Destination port
      seq_t seq_nr;                          // Sequence number
      seq_t ack_nr;                          // Acknowledge number
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
      Pseudo_header pseudo;
      Header tcp;
    }__attribute__((packed));



    /// USER INTERFACE - TCP ///

    /*
      Constructor
    */
    TCP(IPStack&);

    /*
      Bind a new listener to a given Port.
    */
    Connection& bind(port_t port);

    /*
      Active open a new connection to the given remote.
    */
    Connection_ptr connect(Socket remote);

    /*
      Active open a new connection to the given remote.
    */
    inline auto connect(Address address, port_t port = 80) {
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
    inline void set_network_out(downstream del)
    { _network_layer_out = del; }

    /*
      Compute the TCP checksum
    */
    static uint16_t checksum(const Packet_ptr);

    inline const auto& listeners()
    { return listeners_; }

    inline const auto& connections()
    { return connections_; }

    /*
      Number of open ports.
    */
    inline size_t open_ports()
    { return listeners_.size(); }

    /*
      Number of active connections.
    */
    inline size_t active_connections()
    { return connections_.size(); }

    /*
      Maximum Segment Lifetime
    */
    inline auto MSL() const
    { return MAX_SEG_LIFETIME; }

    /*
      Set Maximum Segment Lifetime
    */
    inline void set_MSL(const std::chrono::milliseconds msl)
    { MAX_SEG_LIFETIME = msl; }

    /*
      Maximum Segment Size
      [RFC 793] [RFC 879] [RFC 6691]
    */
    inline constexpr uint16_t MSS() const
    { return network().MDDS() - sizeof(TCP::Header); }

    /*
      Show all connections for TCP as a string.
    */
    std::string to_string() const;

    inline std::string status() const
    { return to_string(); }

    inline size_t writeq_size() const
    { return writeq.size(); }

    inline Address address()
    { return inet_.ip_addr(); }

  private:

    IPStack& inet_;
    std::map<port_t, Connection> listeners_;
    std::map<Connection::Tuple, Connection_ptr> connections_;

    downstream _network_layer_out;

    std::deque<Connection_ptr> writeq;

    /*
      Settings
    */
    port_t current_ephemeral_ = 1024;

    std::chrono::milliseconds MAX_SEG_LIFETIME;

    /*
      Transmit packet to network layer (IP).
    */
    void transmit(Packet_ptr);

    /*
      Generate a unique initial sequence number (ISS).
    */
    static seq_t generate_iss();

    /*
      Returns a free port for outgoing connections.
    */
    port_t next_free_port();

    /*
      Check if the port is in use either among "listeners" or "connections"
    */
    bool port_in_use(const port_t) const;

    /*
      Packet is dropped.
    */
    void drop(Packet_ptr);

    /*
      Add a Connection.
    */
    Connection_ptr add_connection(port_t local_port, Socket remote);

    /*
      Close and delete the connection.
    */
    void close_connection(Connection&);

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
    inline void kick()
    { process_writeq(inet_.transmit_queue_available()); }

    inline IP4& network() const
    { return inet_.ip_obj(); }

  }; // < class TCP

}; // < namespace tcp
}; // < namespace net

#endif // < NET_TCP_TCP_HPP
