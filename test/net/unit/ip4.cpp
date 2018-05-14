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

#include <map>
#include <common.cxx>
#include <nic_mock.hpp>
#include <net/inet>

int ip_packets_dropped = 0;
int ip_packets_received = 0;

net::IP4::Drop_reason last_drop_reason;
net::IP4::Direction last_drop_direction;

std::map<net::IP4::Drop_reason, std::string> drop_reasons {
  {net::IP4::Drop_reason::Bad_source, "Bad source address"},
  {net::IP4::Drop_reason::Bad_destination, "Bad destination address"},
  {net::IP4::Drop_reason::Wrong_version, "Wrong version"},
  {net::IP4::Drop_reason::Wrong_checksum, "Wrong checksum"},
  {net::IP4::Drop_reason::Unknown_proto, "Unknown proto"},
  {net::IP4::Drop_reason::TTL0, "TTL 0"}
};

std::map<net::Protocol, int> pass_count {
  {net::Protocol::UDP, 0},
  {net::Protocol::TCP, 0},
  {net::Protocol::ICMPv4, 0}
};

#define MYINFO(X,...) INFO("Unit IP4", X, ##__VA_ARGS__)

void ip_rcv_udp(net::Packet_ptr) {
  MYINFO("UDP got packet from IP");

  pass_count[net::Protocol::UDP]++;
}

void ip_rcv_tcp(net::Packet_ptr) {
  MYINFO("TCP got packet from IP");

  pass_count[net::Protocol::TCP]++;
}

void ip_rcv_icmp(net::Packet_ptr) {
  MYINFO("ICMP got packet from IP");

  pass_count[net::Protocol::ICMPv4]++;
}

void ip_drop(net::Packet_ptr, net::IP4::Direction dir, net::IP4::Drop_reason reason) {
  last_drop_reason = reason;
  last_drop_direction = dir;
  ip_packets_dropped++;
  MYINFO("Packet dropped. Reason: %s", drop_reasons[reason].c_str());
}

#define EXPECT_DROP(PCKT, DIRECTION, REASON)            \
  {                                                     \
    auto dropcount = ip_packets_dropped;                \
    nic.receive(PCKT);                                  \
    EXPECT(ip_packets_dropped == dropcount + 1);        \
    EXPECT(last_drop_direction == net::IP4::DIRECTION); \
    EXPECT(last_drop_reason == net::IP4::REASON);       \
  }                                                     \

#define EXPECT_PASS(PCKT, PROTO)                        \
  {                                                     \
    auto passed = pass_count[PROTO];                    \
    nic.receive(PCKT);                                  \
    EXPECT(pass_count[PROTO] == passed + 1);            \
  }                                                     \


int sections = 0;

using namespace net;


CASE("IP4 is still a thing")
{

  INFO("Unit", "IP4 is still a thing");

  SETUP("A pre-wired IP4 instance with custom upstream handlers"){

    Nic_mock nic;
    Inet inet{nic};

    inet.ip_obj().set_drop_handler(ip_drop);
    inet.ip_obj().set_udp_handler(ip_rcv_udp);
    inet.ip_obj().set_tcp_handler(ip_rcv_tcp);
    inet.ip_obj().set_icmp_handler(ip_rcv_icmp);

    // Garbage packets

    SECTION("Raw, uninitialized packets get dropped"){
      auto raw_pckt = inet.create_packet();
      EXPECT_DROP(std::move(raw_pckt), Direction::Upstream, Drop_reason::Wrong_version);

      // All packets so far are dropped
      EXPECT(inet.ip_obj().get_packets_rx() == 1u);
      EXPECT(inet.ip_obj().get_packets_dropped() == 1u);
      MYINFO("Section %i done", ++sections);
    }

    SECTION("Inet-created IP packets are initialized correctly"){
      auto ip_pckt1 = inet.create_ip_packet(Protocol::ICMPv4);
      EXPECT(ip_pckt1->ip_version() == 4);
      EXPECT(ip_pckt1->is_ipv4());
      EXPECT(ip_pckt1->ip_ihl() == 5);
      EXPECT(ip_pckt1->ip_header_length() == 20);
      EXPECT(ip_pckt1->ip_dscp() == DSCP::CS0);
      EXPECT(ip_pckt1->ip_ecn() == ECN::NOT_ECT);
      EXPECT(ip_pckt1->ip_total_length() == 20);
      EXPECT(ip_pckt1->ip_id() == 0);
      EXPECT(ip_pckt1->ip_flags() == ip4::Flags::NONE);
      EXPECT(ip_pckt1->ip_frag_offs() == 0);
      EXPECT(ip_pckt1->ip_ttl() == 64);
      EXPECT(ip_pckt1->ip_protocol() == Protocol::ICMPv4);
      EXPECT(ip_pckt1->ip_checksum() == 0);
      EXPECT(ip_pckt1->ip_src() == IP4::ADDR_ANY);
      EXPECT(ip_pckt1->ip_dst() == IP4::ADDR_ANY);
      MYINFO("Section %i done", ++sections);
    }

    //
    // Proper IP packets with invalid header fields are dropped
    //

    SECTION("IP Packets with invalid checksum gets dropped"){

      auto ip_pckt1 = inet.create_ip_packet(Protocol::ICMPv4);
      auto ip_pckt2 = inet.create_ip_packet(Protocol(0));

      // Packets with default (0) checksum gets dropped
      EXPECT_DROP(std::move(ip_pckt1), Direction::Upstream, Drop_reason::Wrong_checksum);

      // Explicitly setting a wrong checksum for packet 1 cause drop as well
      ip_pckt2->set_ip_checksum(987);

      EXPECT_DROP(std::move(ip_pckt2), Direction::Upstream, Drop_reason::Wrong_checksum);

      MYINFO("Section %i done", ++sections);

    }

    SECTION("IP packets with bad destination address gets dropped") {

      // Martian packet gets dropped
      auto ip_pckt = inet.create_ip_packet(Protocol::UDP);
      ip_pckt->set_ip_src({255,255,255,255});
      ip_pckt->make_flight_ready();

      EXPECT_DROP(std::move(ip_pckt), Direction::Upstream, Drop_reason::Bad_source);

      ip_pckt = inet.create_ip_packet(Protocol::UDP);
      ip_pckt->set_ip_src(inet.broadcast_addr());
      ip_pckt->make_flight_ready();

      EXPECT_DROP(std::move(ip_pckt), Direction::Upstream, Drop_reason::Bad_source);

      ip_pckt = inet.create_ip_packet(Protocol::UDP);
      ip_pckt->set_ip_src({224,0,0,10});
      ip_pckt->make_flight_ready();

      EXPECT_DROP(std::move(ip_pckt), Direction::Upstream, Drop_reason::Bad_source);

      ip_pckt = inet.create_ip_packet(Protocol::UDP);
      ip_pckt->set_ip_src({225,1,2,3});
      ip_pckt->make_flight_ready();

      EXPECT_DROP(std::move(ip_pckt), Direction::Upstream, Drop_reason::Bad_source);

      // Corner case source from multicast address gets dropped
      ip_pckt = inet.create_ip_packet(Protocol::UDP);
      ip_pckt->set_ip_src({239,255,255,255});
      ip_pckt->make_flight_ready();

      EXPECT_DROP(std::move(ip_pckt), Direction::Upstream, Drop_reason::Bad_source);

      MYINFO("Section %i done", ++sections);
    }

    SECTION("IP packets with bad source address gets dropped") {

      // Martian packet gets dropped
      auto ip_pckt = inet.create_ip_packet(Protocol::UDP);
      ip_pckt->set_ip_src({192,168,1,100});
      ip_pckt->make_flight_ready();

      // we're not allowed to transmit IP packets without payload
      ip_pckt->set_ip_data_length(20);

      {
        auto dropcount = ip_packets_dropped;
        inet.ip_obj().transmit(std::move(ip_pckt));
        EXPECT(ip_packets_dropped == dropcount + 1);
        EXPECT(last_drop_direction == IP4::Direction::Downstream);
        EXPECT(last_drop_reason == IP4::Drop_reason::Bad_source);
      }

      MYINFO("Section %i done", ++sections);
    }

    SECTION("IP header flags are set correctly")
    {
      auto ip_pckt = inet.create_ip_packet(Protocol::TCP);
      EXPECT(ip_pckt->ip_flags() == ip4::Flags::NONE);

      ip_pckt->set_ip_flags(ip4::Flags::DF);
      EXPECT(ip_pckt->ip_flags() == ip4::Flags::DF);

      ip_pckt->set_ip_flags(ip4::Flags::MF);
      EXPECT(ip_pckt->ip_flags() == ip4::Flags::MF);

      ip_pckt->set_ip_flags(ip4::Flags::MFDF);
      EXPECT(ip_pckt->ip_flags() == ip4::Flags::MFDF);
    }


    //
    // Packet with unknown protocol gets dropped
    //


    SECTION("IP packet with invalid protocol gets dropped"){
      auto ip_pckt = inet.create_ip_packet((Protocol) 16);  // CHAOS
      ip_pckt->set_ip_src({10,0,0,45});
      ip_pckt->make_flight_ready();
      EXPECT_DROP(std::move(ip_pckt), Direction::Upstream, Drop_reason::Unknown_proto);
    }


    //
    // Valid packets are forwarded to proto handlers
    //


    SECTION("Valid UDP packets gets passed on"){
      // Default initialized source and (0) gets through
      // (Might be for DHCPD)
      auto ip_pckt = inet.create_ip_packet(Protocol::UDP);
      ip_pckt->make_flight_ready();
      EXPECT_PASS(std::move(ip_pckt), Protocol::UDP);
    }

    SECTION("Valid TCP packets gets passed on"){
      auto ip_pckt = inet.create_ip_packet(Protocol::TCP);
      ip_pckt->make_flight_ready();
      EXPECT_PASS(std::move(ip_pckt), Protocol::TCP);
      MYINFO("Section %i done", ++sections);
    }
  }
}
