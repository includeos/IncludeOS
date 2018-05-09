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

//#include "packet_icmp6.hpp"
#include <map>
#include <timers>

namespace net
{
  class PacketICMP6;

  class ICMPv6
  {
#if 0
    using ICMP_type = ICMP_error::ICMP_type;
    using ICMP_code = ICMP_error::ICMP_code;

  public:
    using Stack = IP6::Stack;
    using Tuple = std::pair<uint16_t, uint16_t>;  // identifier and sequence number
    using icmp_func = delegate<void(ICMP_view)>;

    static const int SEC_WAIT_FOR_REPLY = 40;

    // Initialize
    ICMPv6(Stack&);

    // Input from network layer
    void receive(Packet_ptr);

    // Delegate output to network layer
    inline void set_network_out(downstream s)
    { network_layer_out_ = s; };

    /**
     *  Destination Unreachable sent from host because of port (UDP) or protocol (IP6) unreachable
     */
    void destination_unreachable(Packet_ptr pckt, icmp6::code::Dest_unreachable code);

    /**
     *
     */
    void redirect(Packet_ptr pckt, icmp6::code::Redirect code);

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

    // May
    void timestamp_request(IP6::addr ip);
    void timestamp_reply(icmp6::Packet& req);

    void ping(IP6::addr ip);
    void ping(IP6::addr ip, icmp_func callback, int sec_wait = SEC_WAIT_FOR_REPLY);

    void ping(const std::string& hostname);
    void ping(const std::string& hostname, icmp_func callback, int sec_wait = SEC_WAIT_FOR_REPLY);
#endif


#if 0
    typedef uint8_t type_t;
    typedef int (*handler_t)(ICMPv6&, std::shared_ptr<PacketICMP6>&);

    ICMPv6(IP6::addr& ip6);

    struct header
    {
      uint8_t  type;
      uint8_t  code;
      uint16_t checksum;
    } __attribute__((packed));

    struct pseudo_header
    {
      IP6::addr src;
      IP6::addr dst;
      uint32_t  len;
      uint8_t   zeros[3];
      uint8_t   next;
    } __attribute__((packed));

    struct echo_header
    {
      uint8_t  type;
      uint8_t  code;
      uint16_t checksum;
      uint16_t identifier;
      uint16_t sequence;
      uint8_t  data[0];
    } __attribute__((packed));

    // packet from IP6 layer
    int bottom(Packet_ptr pckt);

    // set the downstream delegate
    inline void set_ip6_out(IP6::downstream6 del)
    {
      this->ip6_out = del;
    }

    inline const IP6::addr& local_ip()
    {
      return localIP;
    }

    // message types & codes
    static inline bool is_error(uint8_t type)
    {
      return type < 128;
    }
    static std::string code_string(uint8_t type, uint8_t code);

    // calculate checksum of any ICMP message
    static uint16_t checksum(std::shared_ptr<PacketICMP6>& pckt);

    // provide a handler for a @type of ICMPv6 message
    inline void listen(type_t type, handler_t func)
    {
      listeners[type] = func;
    }

    // transmit packet downstream
    int transmit(std::shared_ptr<PacketICMP6>& pckt);

    // send NDP router solicitation
    void discover();

  private:
    std::map<type_t, handler_t> listeners;
    // connection to IP6 layer
    IP6::downstream6 ip6_out;
    // this network stacks IPv6 address
    IP6::addr& localIP;
  };

  class PacketICMP6 : public PacketIP6
  {
  public:
    inline ICMPv6::header& header()
    {
      return *(ICMPv6::header*) this->payload();
    }
    inline const ICMPv6::header& header() const
    {
      return *(ICMPv6::header*) this->payload();
    }

    inline uint8_t type() const
    {
      return header().type;
    }
    inline uint8_t code() const
    {
      return header().code;
    }
    inline uint16_t checksum() const
    {
      return ntohs(header().checksum);
    }

    void set_length(uint32_t icmp_len)
    {
      // new total IPv6 payload length
      ip6_header().set_size(icmp_len);
      // new total packet length
      set_size(sizeof(IP6::full_header) + icmp_len);
    }
#endif
  };
}
#endif
