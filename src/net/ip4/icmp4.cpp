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

// #define DEBUG
#include <net/ip4/icmp4.hpp>

namespace net {

  ICMPv4::ICMPv4(Stack& inet) :
    inet_{inet}
  {}

  void ICMPv4::receive(Packet_ptr pckt) {
    if ((size_t)pckt->size() < sizeof(IP4::header) + icmp4::Packet::header_size()) // Drop if not a full header
      return;

    auto pckt_ip4 = static_unique_ptr_cast<PacketIP4>(std::move(pckt));
    auto req = icmp4::Packet(std::move(pckt_ip4));

    switch(req.type()) {
    case (icmp4::Type::DEST_UNREACHABLE):
      debug("<ICMP> DESTINATION UNREACHABLE from %s\n", req.ip().src().str().c_str());
      // TODO
      break;
    case (icmp4::Type::SRC_QUENCH):
      debug("<ICMP> SOURCE QUENCH from %s\n", req.ip().src().str().c_str());
      // TODO
      break;
    case (icmp4::Type::REDIRECT):
      debug("<ICMP> REDIRECT from %s\n", req.ip().src().str().c_str());
      // TODO
      break;
    case (icmp4::Type::ECHO):
      debug("<ICMP> PING from %s\n", req.ip().src().str().c_str());
      ping_reply(req);
      break;
    case (icmp4::Type::TIME_EXCEEDED):
      debug("<ICMP> TIME EXCEEDED from %s\n", req.ip().src().str().c_str());
      // TODO
      break;
    case (icmp4::Type::PARAMETER_PROBLEM):
      debug("<ICMP> PARAMETER PROBLEM from %s\n", req.ip().src().str().c_str());
      // TODO
      break;
    case (icmp4::Type::TIMESTAMP):
      debug("<ICMP> TIMESTAMP from %s\n", req.ip().src().str().c_str());
      // TODO
      timestamp_reply(req);
      break;
    case (icmp4::Type::TIMESTAMP_REPLY):
      debug("<ICMP> TIMESTAMP REPLY from %s\n", req.ip().src().str().c_str());
      // TODO
      break;
    case (icmp4::Type::INFO_REQUEST):
      debug("<ICMP> INFO REQUEST from %s\n", req.ip().src().str().c_str());
      // TODO
      information_reply(req);
      break;
    case (icmp4::Type::INFO_REPLY):
      debug("<ICMP> INFO REPLY from %s\n", req.ip().src().str().c_str());
      // TODO
      break;
    case (icmp4::Type::ECHO_REPLY):
      debug("<ICMP> PING Reply from %s\n", req.ip().src().str().c_str());
      // TODO
      break;
    }
  }

  void ICMPv4::destination_unreachable(icmp4::Packet& req, icmp4::code::Dest_unreachable code) {
    send_response(req, icmp4::Type::DEST_UNREACHABLE, (uint8_t) code);
  }

  void ICMPv4::time_exceeded(icmp4::Packet& req, icmp4::code::Time_exceeded code) {
    send_response(req, icmp4::Type::TIME_EXCEEDED, (uint8_t) code);
  }

  void ICMPv4::parameter_problem(icmp4::Packet& req) {
    send_response(req, icmp4::Type::PARAMETER_PROBLEM, 0);
  }

  void ICMPv4::source_quench(icmp4::Packet& req) {
    send_response(req, icmp4::Type::SRC_QUENCH, 0);
  }

  void ICMPv4::redirect(icmp4::Packet& req, icmp4::code::Redirect code) {
    send_response(req, icmp4::Type::REDIRECT, (uint8_t) code);
  }

  void ICMPv4::timestamp_request(IP4::addr ip) {
    // TODO
    // send_request(ip, icmp4::Type::TIMESTAMP, 0, icmp4::Packet::Span(, ));
  }

  void ICMPv4::timestamp_reply(icmp4::Packet& req) {
    // TODO
    // send_response(req, icmp4::Type::TIMESTAMP_REPLY, 0);
  }

  void ICMPv4::information_request(IP4::addr ip) {
    // TODO
    // send_request(ip, icmp4::Type::INFO_REQUEST, 0, icmp4::Packet::Span(, ));
  }

  void ICMPv4::information_reply(icmp4::Packet& req) {
    send_response(req, icmp4::Type::INFO_REPLY, 0);
  }

  void ICMPv4::ping_request(IP4::addr ip) {
    send_request(ip, icmp4::Type::ECHO, 0, icmp4::Packet::Span(includeos_payload_, 48));
  }

  void ICMPv4::ping_reply(icmp4::Packet& req) {
    send_response(req, icmp4::Type::ECHO_REPLY, 0);
  }

  void ICMPv4::send_request(IP4::addr dest_ip, icmp4::Type type, uint8_t code, icmp4::Packet::Span payload) {
    // Provision new IP4-packet
    icmp4::Packet req(inet_.ip_packet_factory());

    // Populate request IP header
    req.ip().set_src(inet_.ip_addr());
    req.ip().set_dst(dest_ip);

    // Populate request ICMP header
    req.set_type(icmp4::Type::ECHO);
    req.set_code(code);
    req.set_id(0);
    req.set_sequence(0);

    debug("<ICMP> Transmitting request to %s\n", dest_ip.to_string().c_str());

    // Payload
    req.set_payload(payload);

    // Add checksum
    req.set_checksum();

    debug("<ICMP> Request size: %i payload size: %i, checksum: 0x%x\n",
      req.ip().size(), req.payload().size(), req.compute_checksum());

    network_layer_out_(req.release());
  }

  void ICMPv4::send_response(icmp4::Packet& req, icmp4::Type type, uint8_t code) {
    // Provision new IP4-packet
    icmp4::Packet res(inet_.ip_packet_factory());

    // Populate response IP header
    res.ip().set_src(inet_.ip_addr());
    res.ip().set_dst(req.ip().src());

    // Populate response ICMP header
    res.set_type(type);
    res.set_code(code);
    res.set_id(req.id());
    res.set_sequence(req.sequence());

    debug("<ICMP> Transmitting answer to %s\n", res.ip().dst().str().c_str());

    // Payload
    res.set_payload(req.payload());

    // Add checksum
    res.set_checksum();

    debug("<ICMP> Response size: %i payload size: %i, checksum: 0x%x\n",
          res.ip().size(), res.payload().size(), res.compute_checksum());

    network_layer_out_(res.release());
  }

} //< namespace net
