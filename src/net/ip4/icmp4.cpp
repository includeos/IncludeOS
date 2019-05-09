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

// #define DEBUG
#include <net/ip4/icmp4.hpp>
#include <net/inet>

namespace net {

  // ---------------------------- ICMP_view ----------------------------

  std::string ICMP_view::to_string() const {
    if (type_ == ICMP_type::NO_REPLY)
      return "No reply received";

    return "Identifier: " + std::to_string(id_) + "\n" +
      "Sequence number: " + std::to_string(seq_) + "\n" +
      "Source: " + src_.to_string() + "\n" +
      "Destination: " + dst_.to_string() + "\n" +
      "Type: " + icmp4::get_type_string(type_) + "\n" +
      "Code: " + icmp4::get_code_string(type_, code_) + "\n" +
      "Checksum: " + std::to_string(checksum_) + "\n" +
      "Data: " + payload_;
  }

  // ------------------------------ ICMPv4 ------------------------------

  int ICMPv4::request_id_ = 0;

  ICMPv4::ICMPv4(Stack& inet) :
    inet_{inet}
  {}

  void ICMPv4::receive(Packet_ptr pckt) {
    if (not is_full_header((size_t) pckt->size())) // Drop if not a full header
      return;

    auto pckt_ip4 = static_unique_ptr_cast<PacketIP4>(std::move(pckt));
    auto req = icmp4::Packet(std::move(pckt_ip4));

    switch(req.type()) {
    case ICMP_type::ECHO:
      debug("<ICMP> PING from %s\n", req.ip().ip_src().to_string().c_str());
      ping_reply(req);
      break;
    case ICMP_type::ECHO_REPLY:
      debug("<ICMP> PING Reply from %s\n", req.ip().ip_src().str().c_str());
      execute_ping_callback(req);
      break;
    case ICMP_type::DEST_UNREACHABLE:
      if (req.code() == (uint8_t) icmp4::code::Dest_unreachable::FRAGMENTATION_NEEDED) {
        debug("<ICMP> ICMP Too Big message from %s\n", req.ip().ip_src().str().c_str());
        handle_too_big(req);
        return;
      }
      break; // else continue
    case ICMP_type::REDIRECT:
    case ICMP_type::TIME_EXCEEDED:
    case ICMP_type::PARAMETER_PROBLEM:
      debug("<ICMP> ICMP error message from %s\n", req.ip().ip_src().str().c_str());
      forward_to_transport_layer(req);
      break;
    case ICMP_type::TIMESTAMP:
      debug("<ICMP> TIMESTAMP from %s\n", req.ip().ip_src().str().c_str());
      // TODO May
      // timestamp_reply(req);
      break;
    case ICMP_type::TIMESTAMP_REPLY:
      debug("<ICMP> TIMESTAMP REPLY from %s\n", req.ip().ip_src().str().c_str());
      // TODO May
      break;
    default:  // ICMP_type::NO_REPLY
      return;
    }
  }

  void ICMPv4::forward_to_transport_layer(icmp4::Packet& req) {
    ICMP_error err{req.type(), req.code()};

    // The icmp4::Packet's payload contains the original packet sent that resulted
    // in an error
    int payload_idx = req.payload_index();
    auto packet_ptr = req.release();
    packet_ptr->increment_layer_begin(payload_idx);

    // inet forwards to transport layer (UDP or TCP)
    inet_.error_report(err, std::move(packet_ptr));
  }

  void ICMPv4::handle_too_big(icmp4::Packet& req) {
    // In this type of ICMP packet, the Next-Hop MTU is placed at the same location as
    // the sequence number in an ECHO message f.ex.
    ICMP_error err{req.type(), req.code(), req.sequence()};

    // The icmp4::Packet's payload contains the original packet sent that resulted
    // in the Fragmentation Needed
    int payload_idx = req.payload_index();
    auto packet_ptr = req.release();
    packet_ptr->increment_layer_begin(payload_idx);

    // Inet updates the corresponding Path MTU value in IP and notifies the transport/packetization layer
    inet_.error_report(err, std::move(packet_ptr));
  }

  void ICMPv4::destination_unreachable(Packet_ptr pckt, icmp4::code::Dest_unreachable code) {
    if (not is_full_header((size_t) pckt->size())) // Drop if not a full header
      return;

    auto pckt_ip4 = static_unique_ptr_cast<PacketIP4>(std::move(pckt));

    // Only sending destination unreachable message if the destination IP address is not
    // broadcast or multicast
    if (pckt_ip4->ip_dst() != inet_.broadcast_addr() and pckt_ip4->ip_dst() != IP4::ADDR_BCAST and
      not pckt_ip4->ip_dst().is_multicast()) {
      auto pckt_icmp4 = icmp4::Packet(std::move(pckt_ip4));
      send_response(pckt_icmp4, ICMP_type::DEST_UNREACHABLE, (ICMP_code) code);
    }
  }

  void ICMPv4::redirect(Packet_ptr /* pckt */, icmp4::code::Redirect /* code */) {
    // send_response(req, ICMP_type::REDIRECT, (ICMP_code) code, ...);
  }

  void ICMPv4::time_exceeded(Packet_ptr pckt, icmp4::code::Time_exceeded code) {
    if (not is_full_header((size_t) pckt->size())) // Drop if not a full header
      return;

    auto pckt_ip4 = static_unique_ptr_cast<PacketIP4>(std::move(pckt));
    auto pckt_icmp4 = icmp4::Packet(std::move(pckt_ip4));
    send_response(pckt_icmp4, ICMP_type::TIME_EXCEEDED, (ICMP_code) code);
  }

  void ICMPv4::parameter_problem(Packet_ptr pckt, uint8_t error_pointer) {
    if (not is_full_header((size_t) pckt->size())) // Drop if not a full header
      return;

    auto pckt_ip4 = static_unique_ptr_cast<PacketIP4>(std::move(pckt));
    auto pckt_icmp4 = icmp4::Packet(std::move(pckt_ip4));
    send_response(pckt_icmp4, ICMP_type::PARAMETER_PROBLEM, 0, error_pointer);
  }

  void ICMPv4::timestamp_request(ip4::Addr /* ip */) {
    // TODO
    // send_request(ip, ICMP_type::TIMESTAMP, 0, ...);
  }

  void ICMPv4::timestamp_reply(icmp4::Packet& /* req */) {
    // TODO
    // send_response(req, ICMP_type::TIMESTAMP_REPLY, 0, ...);
  }

  void ICMPv4::ping(ip4::Addr ip)
  { send_request(ip, ICMP_type::ECHO, 0); }

  void ICMPv4::ping(ip4::Addr ip, icmp_func callback, int sec_wait)
  { send_request(ip, ICMP_type::ECHO, 0, callback, sec_wait); }

  void ICMPv4::ping(const std::string& hostname) {
    inet_.resolve(hostname, [this] (dns::Response_ptr res, Error err) {
      if (!err)
      {
        auto addr = res->get_first_ipv4();
        if(addr != 0)
          ping(addr);
      }
    });
  }

  void ICMPv4::ping(const std::string& hostname, icmp_func callback, int sec_wait) {
    inet_.resolve(hostname,
      Inet::resolve_func::make_packed([this, callback, sec_wait]
      (dns::Response_ptr res, Error err)
    {
      if (!err)
      {
        auto addr = res->get_first_ipv4();
        if(addr != 0)
          ping(addr, callback, sec_wait);
      }
    }));
  }

  void ICMPv4::send_request(ip4::Addr dest_ip, ICMP_type type, ICMP_code code,
    icmp_func callback, int sec_wait, uint16_t sequence) {

    // Provision new IP4-packet
    icmp4::Packet req(inet_.ip_packet_factory());

    // Populate request IP header
    req.ip().set_ip_src(inet_.ip_addr());
    req.ip().set_ip_dst(dest_ip);

    uint16_t temp_id = request_id_;
    // Populate request ICMP header
    req.set_type(type);
    req.set_code(code);
    req.set_id(request_id_++);
    req.set_sequence(sequence);

    if (callback) {
      ping_callbacks_.emplace(std::piecewise_construct,
                              std::forward_as_tuple(std::make_pair(temp_id, sequence)),
                              std::forward_as_tuple(ICMP_callback{*this, std::make_pair(temp_id, sequence), callback, sec_wait}));
    }

    debug("<ICMP> Transmitting request to %s\n", dest_ip.to_string().c_str());

    // Payload
    // Default: includeos_payload_
    req.set_payload(icmp4::Packet::Span(includeos_payload_, 48));

    // Add checksum
    req.set_checksum();

    debug("<ICMP> Request size: %i payload size: %i, checksum: 0x%x\n",
      req.ip().size(), req.payload().size(), req.compute_checksum());

    network_layer_out_(req.release());
  }

  void ICMPv4::send_response(icmp4::Packet& req, ICMP_type type, ICMP_code code, uint8_t error_pointer) {
    // Provision new IP4-packet
    icmp4::Packet res(inet_.ip_packet_factory());

    // Populate response IP header
    res.ip().set_ip_src(inet_.ip_addr());
    res.ip().set_ip_dst(req.ip().ip_src());

    // Populate response ICMP header
    res.set_type(type);
    res.set_code(code);

    debug("<ICMP> Transmitting answer to %s\n", res.ip().dst().str().c_str());

    // Payload
    // Default: Header and 64 bits (8 bytes) of original payload
    res.set_payload(req.header_and_data());

    // Add checksum
    res.set_checksum();

    if (error_pointer != std::numeric_limits<uint8_t>::max())
      res.set_pointer(error_pointer);

    debug("<ICMP> Response size: %i payload size: %i, checksum: 0x%x\n",
          res.ip().size(), res.payload().size(), res.compute_checksum());

    network_layer_out_(res.release());
  }

  void ICMPv4::ping_reply(icmp4::Packet& req) {
    // Provision new IP4-packet
    icmp4::Packet res(inet_.ip_packet_factory());

    // drop if the packet is too small
    if (res.ip().capacity() < res.ip().ip_header_length()
     + (int) res.header_size() + req.payload().size())
    {
      printf("WARNING: Network MTU too small for ICMP response, dropping\n");
      return;
    }

    // Populate response IP header
    res.ip().set_ip_src(inet_.ip_addr());
    res.ip().set_ip_dst(req.ip().ip_src());

    // Populate response ICMP header
    res.set_type(ICMP_type::ECHO_REPLY);
    res.set_code(0);
    // Incl. id and sequence number
    res.set_id(req.id());
    res.set_sequence(req.sequence());

    debug("<ICMP> Transmitting answer to %s\n",
          res.ip().ip_dst().str().c_str());

    // Payload
    res.set_payload(req.payload());

    // Add checksum
    res.set_checksum();

    debug("<ICMP> Response size: %i payload size: %i, checksum: 0x%x\n",
          res.ip().size(), res.payload().size(), res.compute_checksum());

    network_layer_out_(res.release());
  }

} //< namespace net
