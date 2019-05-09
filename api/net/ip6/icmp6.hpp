// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015 Oslo and Akershus University College of Applied Sciences
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

#ifndef NET_IP6_ICMPv6_HPP
#define NET_IP6_ICMPv6_HPP

#include "packet_icmp6.hpp"
#include <net/ip6/addr.hpp>
#include <net/inet_common.hpp> // net::downstream
#include <map>
#include <timers>

namespace net
{
  class Inet;
  /**
   *  User friendly ICMP packet (view) used in ping callback (icmp_func)
   */
  class ICMP6_view {

    using ICMP_type = ICMP6_error::ICMP_type;
    using ICMP_code = ICMP6_error::ICMP_code;

  public:
    ICMP6_view() {}

    ICMP6_view(icmp6::Packet& pckt)
    : id_{pckt.id()},
      seq_{pckt.sequence()},
      src_{pckt.ip().ip_src()},
      dst_{pckt.ip().ip_dst()},
      type_{pckt.type()},
      code_{pckt.code()},
      checksum_{pckt.checksum()},
      payload_{(const char*) pckt.payload().data() +sizeof(icmp6::Packet::IdSe), (size_t) pckt.payload().size()-sizeof(icmp6::Packet::IdSe)}
    {}

    uint16_t id() const noexcept
    { return id_; }

    uint16_t seq() const noexcept
    { return seq_; }

    ip6::Addr src() const noexcept
    { return src_; }

    ip6::Addr dst() const noexcept
    { return dst_; }

    ICMP_type type() const noexcept
    { return type_; }

    ICMP_code code() const noexcept
    { return code_; }

    uint16_t checksum() const noexcept
    { return checksum_; }

    std::string payload() const
    { return payload_; }

    operator bool() const noexcept
    { return type_ != ICMP_type::NO_REPLY; }

    std::string to_string() const;

  private:
    uint16_t      id_{0};
    uint16_t      seq_{0};
    ip6::Addr     src_{0,0,0,0};
    ip6::Addr     dst_{0,0,0,0};
    ICMP_type     type_{ICMP_type::NO_REPLY};
    uint8_t       code_{0};
    uint16_t      checksum_{0};
    std::string   payload_{""};

  }; // < class ICMP6_view

  class ICMPv6
  {
    using ICMP_type = ICMP6_error::ICMP_type;
    using ICMP_code = ICMP6_error::ICMP_code;

  public:
    using Stack = Inet;
    using Tuple = std::pair<uint16_t, uint16_t>;  // identifier and sequence number
    using icmp_func = delegate<void(ICMP6_view)>;
    using Route_checker = delegate<bool(ip6::Addr)>;

    static const int SEC_WAIT_FOR_REPLY = 40;

    // Initialize
    ICMPv6(Stack&);

    // Input from network layer
    void receive(Packet_ptr);

    // Delegate output to network layer
    inline void set_network_out(downstream s)
    {
      network_layer_out_ = s;
    }

    void set_ndp_handler(upstream s)
    { ndp_upstream_ = s; }

    void set_mld_handler(upstream s)
    { mld_upstream_ = s; }

    /**
     *  Destination Unreachable sent from host because of port (UDP) or protocol (IP6) unreachable
     */
    void destination_unreachable(Packet_ptr pckt, icmp6::code::Dest_unreachable code);

    /**
     *
     */
    //void redirect(Packet_ptr pckt, icmp6::code::Redirect code);

    /**
     *  Sending a Time Exceeded message from a host when fragment reassembly time exceeded (code 1)
     *  Sending a Time Exceeded message from a gateway when time to live exceeded in transit (code 0)
     */
    void time_exceeded(Packet_ptr pckt, icmp6::code::Time_exceeded code);

    /**
     *  Sending a Parameter Problem message if the gateway or host processing a datagram finds a problem with
     *  the header parameters such that it cannot complete processing the datagram. The message is only sent if
     *  the error caused the datagram to be discarded
     *  Code 0 means Pointer (uint8_t after checksum) indicates the error/identifies the octet where an error was detected
     *  in the IP header
     *  Code 1 means that a required option is missing
     */
    void parameter_problem(Packet_ptr pckt, uint8_t error_pointer);

    void timestamp_request(ip6::Addr ip);
    void timestamp_reply(icmp6::Packet& req);

    void ping(ip6::Addr ip);
    void ping(ip6::Addr ip, icmp_func callback, int sec_wait = SEC_WAIT_FOR_REPLY);

    void ping(const std::string& hostname);
    void ping(const std::string& hostname, icmp_func callback, int sec_wait = SEC_WAIT_FOR_REPLY);

  private:
    static int request_id_; // message identifier for messages originating from IncludeOS
    Stack& inet_;
    downstream network_layer_out_ = nullptr;
    upstream   ndp_upstream_      = nullptr;
    upstream   mld_upstream_      = nullptr;

    inline bool is_full_header(size_t pckt_size)
    { return (pckt_size >= sizeof(ip6::Header) + icmp6::Packet::header_size()); }

    struct ICMP_callback {
      using icmp_func = ICMPv6::icmp_func;
      using Tuple = ICMPv6::Tuple;

      Tuple tuple;
      icmp_func callback;
      Timers::id_t timer_id;

      ICMP_callback(ICMPv6& icmp, Tuple t, icmp_func cb, int sec_wait)
      : tuple{t}, callback{cb}
      {
        timer_id = Timers::oneshot(std::chrono::seconds(sec_wait), [&icmp, t](Timers::id_t) {
          icmp.remove_ping_callback(t);
        });
      }
    };  // < struct ICMP_callback

    std::map<Tuple, ICMP_callback> ping_callbacks_;

    void forward_to_transport_layer(icmp6::Packet& req);

    /**
     *  If a Destination Unreachable: Fragmentation Needed error mesage is received, the Path MTUs in
     *  IP should be updated and the packetization/transportation layer should be notified (through Inet)
     *  RFC 1191 (Path MTU Discovery IP4), 1981 (Path MTU Discovery IP6) and 4821 (Packetization Layer Path MTU Discovery)
     *  A Fragmentation Needed error message is sent as a response to a packet with an MTU that is too big
     *  for a node in the path to its destination and with the Don't Fragment bit set as well
     */
    void handle_too_big(icmp6::Packet& req);

    /**
     *  Find the ping-callback that this packet is a response to, execute it and erase the object
     *  from the ping_callbacks_ map
     */
    void execute_ping_callback(icmp6::Packet& ping_response);

    /** Remove ICMP_callback from ping_callbacks_ map when its timer timeouts */
    void remove_ping_callback(Tuple key);

    void send_request(ip6::Addr dest_ip, ICMP_type type, ICMP_code code,
      icmp_func callback = nullptr, int sec_wait = SEC_WAIT_FOR_REPLY, uint16_t sequence = 0);

    /** Send response without id and sequence number */
    void send_response(icmp6::Packet& req, ICMP_type type, ICMP_code code,
      uint8_t error_pointer = std::numeric_limits<uint8_t>::max());

    /**
     *  Responding to a ping (echo) request
     *  Called from receive-method
     */
    void ping_reply(icmp6::Packet&);

    /* Receive neighbor solicitaion and respond with neighbor advertisement */
    int receive_neighbor_solicitation(icmp6::Packet&);
    void send_router_solicitation();
  };
}
#endif
