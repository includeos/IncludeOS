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

#ifndef NET_IP4_ICMPv4_HPP
#define NET_IP4_ICMPv4_HPP

#include "packet_icmp4.hpp"

namespace net {

  struct ICMPv4 {

    using Stack = IP4::Stack;

    // Initialize
    ICMPv4(Stack&);

    // Input from network layer
    void receive(Packet_ptr);

    // Delegate output to network layer
    inline void set_network_out(downstream s)
    { network_layer_out_ = s;  };

    void destination_unreachable(Packet_ptr pckt, icmp4::code::Dest_unreachable code);

    void redirect(icmp4::Packet& req, icmp4::code::Redirect code);

    void time_exceeded(icmp4::Packet& req, icmp4::code::Time_exceeded code);

    void parameter_problem(icmp4::Packet& req);

    // May
    void timestamp_request(IP4::addr ip);
    void timestamp_reply(icmp4::Packet& req);

    void ping(IP4::addr ip);

  private:

    Stack& inet_;
    downstream network_layer_out_ =   nullptr;
    uint8_t includeos_payload_[48] =  {'I','N','C','L','U','D',
                                      'E','O','S',1,2,3,4,5,
                                      'A','B','C','D','E','F','G','H',
                                      'I','J','K','L','M','N','O','P',
                                      'Q','R','S','T','U','V','W','X',
                                      'Y','Z',1,2,3,4,5,6,
                                      7,8};
    static int id_;

    void ping_reply(icmp4::Packet&);

    void send_request(IP4::addr dest_ip, icmp4::Type type, uint8_t code, icmp4::Packet::Span payload,
      uint16_t sequence = 0);
    void send_response(icmp4::Packet& req, icmp4::Type type, uint8_t code, icmp4::Packet::Span payload);

  }; //< class ICMPv4
} //< namespace net

#endif //< NET_IP4_ICMPv4_HPP
