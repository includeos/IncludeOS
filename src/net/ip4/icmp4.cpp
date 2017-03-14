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

  // ---------------------------- ICMP_packet ----------------------------

  std::string ICMP_packet::to_string() {
    if (not is_reply())
      return "No reply received";

    const std::string t = [&]() {
      switch (type_) {
        using namespace icmp4;
        case Type::ECHO_REPLY: return "ECHO REPLY";
        case Type::DEST_UNREACHABLE: return "DESTINATION UNREACHABLE";
        case Type::REDIRECT: return "REDIRECT";
        case Type::ECHO: return "ECHO";
        case Type::TIME_EXCEEDED: return "TIME EXCEEDED";
        case Type::PARAMETER_PROBLEM: return "PARAMETER PROBLEM";
        case Type::TIMESTAMP: return "TIMESTAMP";
        case Type::TIMESTAMP_REPLY: return "TIMESTAMP REPLY";
        case Type::NO_REPLY: return "NO REPLY";
      }
    }();

    return "Identifier: " + std::to_string(id_) + "\n" +
      "Sequence number: " + std::to_string(seq_) + "\n" +
      "Source: " + src_.to_string() + "\n" +
      "Destination: " + dst_.to_string() + "\n" +
      "Type: " + t + "\n" +
      "Code: " + std::to_string(code_) + "\n" +
      "Checksum: " + std::to_string(checksum_) + "\n" +
      "Data: " + std::string{payload_.begin(), payload_.end()};
  }

  // ------------------------------ ICMPv4 ------------------------------

  int ICMPv4::request_id_ = 0;

  ICMPv4::ICMPv4(Stack& inet) :
    inet_{inet}
  {}

  void ICMPv4::receive(Packet_ptr pckt) {
    if ((size_t)pckt->size() < sizeof(IP4::header) + icmp4::Packet::header_size()) // Drop if not a full header
      return;

    auto pckt_ip4 = static_unique_ptr_cast<PacketIP4>(std::move(pckt));
    auto req = icmp4::Packet(std::move(pckt_ip4));
    std::map<Tuple, ICMP_callback>::iterator it;

    switch(req.type()) {
    case (icmp4::Type::ECHO_REPLY):
      debug("<ICMP> PING Reply from %s\n", req.ip().src().str().c_str());
      execute_ping_callback(req);
      break;
    case (icmp4::Type::DEST_UNREACHABLE):
      debug("<ICMP> DESTINATION UNREACHABLE from %s\n", req.ip().src().str().c_str());
      // TODO
      // Send to transport layer
      break;
    case (icmp4::Type::REDIRECT):
      debug("<ICMP> REDIRECT from %s\n", req.ip().src().str().c_str());
      // TODO
      // Only sent by gateways. Incoming: Update routing information based on the message
      break;
    case (icmp4::Type::ECHO):
      printf("<ICMP> PING from %s\n", req.ip().src().str().c_str());
      ping_reply(req);
      break;
    case (icmp4::Type::TIME_EXCEEDED):
      debug("<ICMP> TIME EXCEEDED from %s\n", req.ip().src().str().c_str());
      // TODO
      // Send to transport layer
      break;
    case (icmp4::Type::PARAMETER_PROBLEM):
      debug("<ICMP> PARAMETER PROBLEM from %s\n", req.ip().src().str().c_str());
      // TODO
      // Send to transport layer
      break;
    case (icmp4::Type::TIMESTAMP):
      debug("<ICMP> TIMESTAMP from %s\n", req.ip().src().str().c_str());
      // TODO May
      // timestamp_reply(req);
      break;
    case (icmp4::Type::TIMESTAMP_REPLY):
      debug("<ICMP> TIMESTAMP REPLY from %s\n", req.ip().src().str().c_str());
      // TODO May
      break;
    default:
      return;
    }
  }

  void ICMPv4::destination_unreachable(Packet_ptr pckt, icmp4::code::Dest_unreachable code) {
    if ((size_t)pckt->size() < sizeof(IP4::header) + icmp4::Packet::header_size()) // Drop if not a full header
      return;
    auto pckt_ip4 = static_unique_ptr_cast<PacketIP4>(std::move(pckt));
    auto pckt_icmp4 = icmp4::Packet(std::move(pckt_ip4));
    send_response(pckt_icmp4, icmp4::Type::DEST_UNREACHABLE, (uint8_t) code, pckt_icmp4.header_and_data());
  }

  void ICMPv4::redirect(icmp4::Packet& /* req */, icmp4::code::Redirect /* code */) {
    // send_response(req, icmp4::Type::REDIRECT, (uint8_t) code, icmp4::Packet::Span(, ));
  }

  void ICMPv4::time_exceeded(Packet_ptr pckt, icmp4::code::Time_exceeded code) {
    if ((size_t)pckt->size() < sizeof(IP4::header) + icmp4::Packet::header_size()) // Drop if not a full header
      return;
    auto pckt_ip4 = static_unique_ptr_cast<PacketIP4>(std::move(pckt));
    auto pckt_icmp4 = icmp4::Packet(std::move(pckt_ip4));
    send_response(pckt_icmp4, icmp4::Type::TIME_EXCEEDED, (uint8_t) code, pckt_icmp4.header_and_data());
  }

  void ICMPv4::parameter_problem(Packet_ptr pckt, uint8_t error) {
    if ((size_t)pckt->size() < sizeof(IP4::header) + icmp4::Packet::header_size()) // Drop if not a full header
      return;
    auto pckt_ip4 = static_unique_ptr_cast<PacketIP4>(std::move(pckt));
    auto pckt_icmp4 = icmp4::Packet(std::move(pckt_ip4));
    send_response(pckt_icmp4, icmp4::Type::PARAMETER_PROBLEM, 0, pckt_icmp4.header_and_data(), error);
  }

  void ICMPv4::timestamp_request(IP4::addr /* ip */) {
    // TODO
    // send_request(ip, icmp4::Type::TIMESTAMP, 0, icmp4::Packet::Span(, ));
  }

  void ICMPv4::timestamp_reply(icmp4::Packet& /* req */) {
    // TODO
    // send_response(req, icmp4::Type::TIMESTAMP_REPLY, 0, icmp4::Packet::Span(, ));
  }

  void ICMPv4::ping(IP4::addr ip) {
    send_request(ip, icmp4::Type::ECHO, 0, icmp4::Packet::Span(includeos_payload_, 48));
  }
  void ICMPv4::ping(IP4::addr ip, icmp_func callback) {
    send_request(ip, icmp4::Type::ECHO, 0, icmp4::Packet::Span(includeos_payload_, 48), callback);
  }

  void ICMPv4::ping_reply(icmp4::Packet& req) {
    send_response_with_id(req, icmp4::Type::ECHO_REPLY, 0, req.payload());
  }

  void ICMPv4::send_request(IP4::addr dest_ip, icmp4::Type type, uint8_t code, icmp4::Packet::Span payload,
    icmp_func callback, uint16_t sequence) {

    // Provision new IP4-packet
    icmp4::Packet req(inet_.ip_packet_factory());

    // Populate request IP header
    req.ip().set_src(inet_.ip_addr());
    req.ip().set_dst(dest_ip);

    uint16_t temp_id = request_id_;
    // Populate request ICMP header
    req.set_type(type);
    req.set_code(code);
    req.set_id(request_id_++);
    req.set_sequence(sequence);

    if (callback) {
      ping_callbacks_.emplace(std::piecewise_construct,
                              std::forward_as_tuple(std::make_pair(temp_id, sequence)),
                              std::forward_as_tuple(ICMP_callback{*this, std::make_pair(temp_id, sequence), callback}));
    }

    debug("<ICMP> Transmitting request to %s\n", dest_ip.to_string().c_str());

    // Payload
    req.set_payload(payload);

    // Add checksum
    req.set_checksum();

    debug("<ICMP> Request size: %i payload size: %i, checksum: 0x%x\n",
      req.ip().size(), req.payload().size(), req.compute_checksum());

    network_layer_out_(req.release());
  }

  void ICMPv4::send_response(icmp4::Packet& req, icmp4::Type type, uint8_t code, icmp4::Packet::Span payload, uint8_t error) {
    // Provision new IP4-packet
    icmp4::Packet res(inet_.ip_packet_factory());

    // Populate response IP header
    res.ip().set_src(inet_.ip_addr());
    res.ip().set_dst(req.ip().src());

    // Populate response ICMP header
    res.set_type(type);
    res.set_code(code);

    debug("<ICMP> Transmitting answer to %s\n", res.ip().dst().str().c_str());

    // Payload
    res.set_payload(payload);

    // Add checksum
    res.set_checksum();

    if (error != 255)
      res.set_pointer(error);

    debug("<ICMP> Response size: %i payload size: %i, checksum: 0x%x\n",
          res.ip().size(), res.payload().size(), res.compute_checksum());

    network_layer_out_(res.release());
  }

  void ICMPv4::send_response_with_id(icmp4::Packet& req, icmp4::Type type, uint8_t code, icmp4::Packet::Span payload) {
    // Provision new IP4-packet
    icmp4::Packet res(inet_.ip_packet_factory());

    // Populate response IP header
    res.ip().set_src(inet_.ip_addr());
    res.ip().set_dst(req.ip().src());

    // Populate response ICMP header
    res.set_type(type);
    res.set_code(code);
    // Incl. id and sequence number
    res.set_id(req.id());
    res.set_sequence(req.sequence());

    debug("<ICMP> Transmitting answer to %s\n", res.ip().dst().str().c_str());

    // Payload
    res.set_payload(payload);

    // Add checksum
    res.set_checksum();

    debug("<ICMP> Response size: %i payload size: %i, checksum: 0x%x\n",
          res.ip().size(), res.payload().size(), res.compute_checksum());

    network_layer_out_(res.release());
  }

} //< namespace net
