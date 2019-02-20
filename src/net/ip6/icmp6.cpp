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

//#define ICMP6_DEBUG 1
#ifdef ICMP6_DEBUG
#define PRINT(fmt, ...) printf(fmt, ##__VA_ARGS__)
#else
#define PRINT(fmt, ...) /* fmt */
#endif
#include <net/ip6/icmp6.hpp>

#include <net/inet>

#include <iostream>
#include <assert.h>

namespace net
{
  static constexpr std::array<uint8_t, 48> includeos_payload =
    {'I','N','C','L','U','D','E','O',
     'S','1','2','3','4','5','A','B',
     'C','D','E','F','G','H','I','J',
     'K','L','M','N','O','P','Q','R',
     'S','T','U','V','W','X','Y','Z',
     '1','2','3','4','5','6','7','8'};
  // ---------------------------- ICMP_view ----------------------------

  std::string ICMP6_view::to_string() const {
    if (type_ == ICMP_type::NO_REPLY)
      return "No reply received";

    return "Identifier: " + std::to_string(id_) + "\n" +
      "Sequence number: " + std::to_string(seq_) + "\n" +
      "Source: " + src_.to_string() + "\n" +
      "Destination: " + dst_.to_string() + "\n" +
      "Type: " + icmp6::get_type_string(type_) + "\n" +
      "Code: " + icmp6::get_code_string(type_, code_) + "\n" +
      "Checksum: " + std::to_string(checksum_) + "\n" +
      "Data: " + payload_+"\n";
  }

  // ------------------------------ ICMPv6 ------------------------------

  int ICMPv6::request_id_ = 0;

  ICMPv6::ICMPv6(Stack& inet) :
    inet_{inet} {}

  void ICMPv6::receive(Packet_ptr pckt) {
    if (not is_full_header((size_t) pckt->size())) // Drop if not a full header
      return;

    auto pckt_ip6 = static_unique_ptr_cast<PacketIP6>(std::move(pckt));
    auto req = icmp6::Packet(std::move(pckt_ip6));
    PRINT("<ICMP6> receive from %s\n", req.ip().ip_src().to_string().c_str());

    switch(req.type()) {
    case (ICMP_type::ECHO):
      PRINT("<ICMP6> PING from %s\n", req.ip().ip_src().to_string().c_str());
      ping_reply(req);
      break;
    case (ICMP_type::ECHO_REPLY):
      PRINT("<ICMP6> PING Reply from %s\n", req.ip().ip_src().str().c_str());
      execute_ping_callback(req);
      break;
    case (ICMP_type::DEST_UNREACHABLE):
      PRINT("<ICMP6> Destination unreachable from %s\n", req.ip().ip_src().str().c_str());
      break;
    case (ICMP_type::PACKET_TOO_BIG):
      PRINT("<ICMP6> ICMP Too Big message from %s\n", req.ip().ip_src().str().c_str());
      handle_too_big(req);
      return;
    case (ICMP_type::TIME_EXCEEDED):
    case (ICMP_type::PARAMETER_PROBLEM):
      PRINT("<ICMP6> ICMP error message from %s\n", req.ip().ip_src().str().c_str());
      forward_to_transport_layer(req);
      break;
    case (ICMP_type::MULTICAST_LISTENER_QUERY):
    case (ICMP_type::MULTICAST_LISTENER_REPORT):
    case (ICMP_type::MULTICAST_LISTENER_DONE):
    case (ICMP_type::MULTICAST_LISTENER_REPORT_v2):
      PRINT("<ICMP6> ICMP MLD from %s\n",
          req.ip().ip_src().str().c_str());
      mld_upstream_(req.release());
      break;
    case (ICMP_type::ND_ROUTER_SOL):
    case (ICMP_type::ND_ROUTER_ADV):
    case (ICMP_type::ND_NEIGHBOUR_SOL):
    case (ICMP_type::ND_NEIGHBOUR_ADV):
    case (ICMP_type::ND_REDIRECT):
      PRINT("<ICMP6> NDP message from %s\n", req.ip().ip_src().str().c_str());
      ndp_upstream_(req.release());
      break;
    case (ICMP_type::ROUTER_RENUMBERING):
      PRINT("<ICMP6> ICMP Router re-numbering message from %s\n", req.ip().ip_src().str().c_str());
      break;
    case (ICMP_type::INFORMATION_QUERY):
    case (ICMP_type::INFORMATION_RESPONSE):
      break;
    default:  // ICMP_type::NO_REPLY
      return;
    }
  }

  void ICMPv6::forward_to_transport_layer(icmp6::Packet& req) {
    ICMP6_error err{req.type(), req.code()};

    // store index before releasing the packet
    const int payload_index = req.payload_index();
    // The icmp6::Packet's payload contains the original packet sent that resulted
    // in an error
    auto packet_ptr = req.release();
    packet_ptr->increment_layer_begin(payload_index);

    // inet forwards to transport layer (UDP or TCP)
    inet_.error_report(err, std::move(packet_ptr));
  }

  void ICMPv6::handle_too_big(icmp6::Packet& req) {
    // In this type of ICMP packet, the Next-Hop MTU is placed at the same location as
    // the sequence number in an ECHO message f.ex.
    ICMP6_error err{req.type(), req.code(), req.sequence()};

    // store index before releasing the packet
    const int payload_index = req.payload_index();
    // The icmp6::Packet's payload contains the original packet sent that resulted
    // in the Fragmentation Needed
    auto packet_ptr = req.release();
    packet_ptr->increment_layer_begin(payload_index);

    // Inet updates the corresponding Path MTU value in IP and notifies the transport/packetization layer
    inet_.error_report(err, std::move(packet_ptr));
  }

   void ICMPv6::execute_ping_callback(icmp6::Packet& ping_response)
   {
    // Find callback matching the reply
    const auto& id_se = ping_response.view_payload_as<icmp6::Packet::IdSe>();
    auto it = ping_callbacks_.find(std::make_pair(id_se.id(), id_se.seq()));

    if (it != ping_callbacks_.end()) {
      it->second.callback(ICMP6_view{ping_response});
      Timers::stop(it->second.timer_id);
      ping_callbacks_.erase(it);
    }
  }

  /** Remove ICMP_callback from ping_callbacks_ map when its timer timeouts */
  void ICMPv6::remove_ping_callback(Tuple key)
  {
    auto it = ping_callbacks_.find(key);

    if (it != ping_callbacks_.end()) {
      // Data back to user if no response found
      it->second.callback(ICMP6_view{});
      Timers::stop(it->second.timer_id);
      ping_callbacks_.erase(it);
    }
  }

  void ICMPv6::destination_unreachable(Packet_ptr pckt, icmp6::code::Dest_unreachable code) {
    if (not is_full_header((size_t) pckt->size())) // Drop if not a full header
      return;

    auto pckt_ip6 = static_unique_ptr_cast<PacketIP6>(std::move(pckt));

#if 0
    // Only sending destination unreachable message if the destination IP address is not
    // broadcast or multicast
    if (pckt_ip6->ip_dst() != inet_.broadcast_addr() and pckt_ip6->ip_dst() != IP6::ADDR_BCAST and
      not pckt_ip6->ip_dst().is_multicast()) {
      auto pckt_icmp6 = icmp6::Packet(std::move(pckt_ip6));
      send_response(pckt_icmp6, ICMP_type::DEST_UNREACHABLE, (ICMP_code) code);
    }
#endif
  }

  void ICMPv6::time_exceeded(Packet_ptr pckt, icmp6::code::Time_exceeded code) {
    if (not is_full_header((size_t) pckt->size())) // Drop if not a full header
      return;

    auto pckt_ip6 = static_unique_ptr_cast<PacketIP6>(std::move(pckt));
    auto pckt_icmp6 = icmp6::Packet(std::move(pckt_ip6));
    send_response(pckt_icmp6, ICMP_type::TIME_EXCEEDED, (ICMP_code) code);
  }

  void ICMPv6::parameter_problem(Packet_ptr pckt, uint8_t error_pointer) {
    if (not is_full_header((size_t) pckt->size())) // Drop if not a full header
      return;

    auto pckt_ip6 = static_unique_ptr_cast<PacketIP6>(std::move(pckt));
    auto pckt_icmp6 = icmp6::Packet(std::move(pckt_ip6));
    send_response(pckt_icmp6, ICMP_type::PARAMETER_PROBLEM, 0, error_pointer);
  }

  void ICMPv6::ping(ip6::Addr ip)
  { send_request(ip, ICMP_type::ECHO, 0); }

  void ICMPv6::ping(ip6::Addr ip, icmp_func callback, int sec_wait)
  { send_request(ip, ICMP_type::ECHO, 0, callback, sec_wait); }

  void ICMPv6::ping(const std::string& hostname) {
#if 0
    inet_.resolve(hostname, [this] (ip6::Addr a, Error err) {
      if (!err and a != IP6::ADDR_ANY)
        ping(a);
    });
#endif
  }

  void ICMPv6::ping(const std::string& hostname, icmp_func callback, int sec_wait) {
#if 0
    inet_.resolve(hostname, Inet::resolve_func::make_packed([this, callback, sec_wait] (ip6::Addr a, Error err) {
      if (!err and a != IP6::ADDR_ANY)
        ping(a, callback, sec_wait);
    }));
#endif
  }

  void ICMPv6::send_request(ip6::Addr dest_ip, ICMP_type type, ICMP_code code,
    icmp_func callback, int sec_wait, uint16_t sequence) {

    // Check if inet is configured with ipv6
    auto src = inet_.ip6_src(dest_ip);
    if (src == ip6::Addr::addr_any) {
      PRINT("<ICMP6> inet is not configured to send ipv6 packets\n");
      return;
    }
    // Provision new IP6-packet
    icmp6::Packet req(inet_.ip6_packet_factory());

    // Populate request IP header
    req.ip().set_ip_src(src);
    req.ip().set_ip_dst(dest_ip);

    uint16_t temp_id = request_id_++;
    // Populate request ICMP header
    req.set_type(type);
    req.set_code(code);

    auto& id_se = req.emplace<icmp6::Packet::IdSe>();
    id_se.set_id(temp_id);
    id_se.set_seq(sequence);

    if (callback) {
      ping_callbacks_.emplace(std::piecewise_construct,
                              std::forward_as_tuple(std::make_pair(temp_id, sequence)),
                              std::forward_as_tuple(ICMP_callback{*this, std::make_pair(temp_id, sequence), callback, sec_wait}));
    }

    PRINT("<ICMP6> Transmitting request to %s\n", dest_ip.to_string().c_str());

    // Default payload
    req.add_payload(includeos_payload.data(), includeos_payload.size());

    // Add checksum
    req.set_checksum();

    PRINT("<ICMP6> Request size: %i payload size: %i, checksum: 0x%x\n",
      req.ip().size(), req.payload().size(), req.compute_checksum());

    network_layer_out_(req.release());
  }

  void ICMPv6::send_response(icmp6::Packet& req, ICMP_type type, ICMP_code code, uint8_t error_pointer) {

    // Check if inet is configured with ipv6
    if (!inet_.is_configured_v6()) {
      PRINT("<ICMP6> inet is not configured to send ipv6 response\n");
      return;
    }

    // Provision new IP6-packet
    icmp6::Packet res(inet_.ip6_packet_factory());

    // Populate response IP header
    res.ip().set_ip_src(inet_.ip6_addr());
    res.ip().set_ip_dst(req.ip().ip_src());

    // Populate response ICMP header
    res.set_type(type);
    res.set_code(code);

    PRINT("<ICMP6> Transmitting answer to %s\n", res.ip().ip_dst().str().c_str());

    // Payload
    // Default: Header and 66 bits (8 bytes) of original payload
    res.add_payload(req.header_and_data().data(), req.header_and_data().size());

    // Add checksum
    res.set_checksum();

    if (error_pointer != std::numeric_limits<uint8_t>::max())
      res.set_pointer(error_pointer);

    PRINT("<ICMP6> Response size: %i payload size: %i, checksum: 0x%x\n",
          res.ip().size(), res.payload().size(), res.compute_checksum());

    network_layer_out_(res.release());
  }

  void ICMPv6::ping_reply(icmp6::Packet& req) {
    // Provision new IP6-packet
    icmp6::Packet res(inet_.ip6_packet_factory());

    // drop if the packet is too small
    if (res.ip().capacity() < IP6_HEADER_LEN
     + (int) res.header_size() + req.payload().size())
    {
      PRINT("WARNING: Network MTU too small for ICMP response, dropping\n");
      return;
    }

    auto dest = req.ip().ip_src();
    // Populate response IP header
    res.ip().set_ip_src(inet_.ip6_src(dest));
    res.ip().set_ip_dst(dest);

    // Populate response ICMP header
    res.set_type(ICMP_type::ECHO_REPLY);
    res.set_code(0);

    const auto& ping = req.view_payload_as<icmp6::Packet::IdSe>();
    // Incl. id and sequence number
    auto& id_se = res.emplace<icmp6::Packet::IdSe>();
    id_se.set_id(ping.id());
    id_se.set_seq(ping.seq());

    PRINT("<ICMP6> Transmitting answer to %s\n",
          res.ip().ip_dst().str().c_str());

    // Payload
    // TODO: since id and seq is part of the payload
    // we need to make some offset stuff here...
    res.add_payload(req.payload().data() + sizeof(icmp6::Packet::IdSe),
      req.payload().size() - sizeof(icmp6::Packet::IdSe));

    // Add checksum
    res.set_checksum();

    PRINT("<ICMP6> Response size: %i payload size: %i, checksum: 0x%x\n",
          res.ip().size(), res.payload().size(), res.compute_checksum());

    network_layer_out_(res.release());
  }
}
