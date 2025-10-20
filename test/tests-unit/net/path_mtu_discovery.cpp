// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2017 Oslo and Akershus University College of Applied Sciences
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

#include <common.cxx>
#include <nic_mock.hpp>
#include <net/inet>

using namespace net;

//
// Unit tests for when Path MTU Discovery is disabled by default:
//
CASE("Path MTU Discovery is disabled by default and can be enabled")
{
  Nic_mock nic;
  Inet inet{nic};
  EXPECT(not inet.ip_obj().path_mtu_discovery());

  inet.set_path_mtu_discovery(true);
  EXPECT(inet.ip_obj().path_mtu_discovery());
}

CASE("An ICMP Too Big message with a next hop MTU value of 45 is invalid")
{
  Nic_mock nic;
  Inet inet{nic};

  inet.set_path_mtu_discovery(true);

  ICMP_error err{icmp4::Type::DEST_UNREACHABLE, (uint8_t) icmp4::code::Dest_unreachable::FRAGMENTATION_NEEDED, 45};
  bool too_big = err.is_too_big();

  auto orig_pckt = inet.create_ip_packet(Protocol::UDP);
  orig_pckt->set_ip_src({10,0,0,45});
  orig_pckt->set_ip_dst({10,0,0,50});
  orig_pckt->set_ip_total_length(50);
  auto total_length = orig_pckt->ip_total_length();

  auto udp_pckt = static_unique_ptr_cast<PacketUDP>(std::move(orig_pckt));
  udp_pckt->set_src_port(80);
  udp_pckt->set_dst_port(80);
  Socket dest{udp_pckt->ip_dst(), udp_pckt->dst_port()};

  inet.ip_obj().update_path(dest, err.pmtu(), too_big, total_length);
  err.set_pmtu(inet.ip_obj().pmtu(dest));

  EXPECT(err.pmtu() == 0);  // Invalid PMTU value 45
}

CASE("An ICMP Too Big message with a next hop MTU value of 68 is valid")
{
  Nic_mock nic;
  Inet inet{nic};

  inet.set_path_mtu_discovery(true);

  ICMP_error err{icmp4::Type::DEST_UNREACHABLE, (uint8_t) icmp4::code::Dest_unreachable::FRAGMENTATION_NEEDED, 68};
  bool too_big = err.is_too_big();

  auto orig_pckt = inet.create_ip_packet(Protocol::UDP);
  orig_pckt->set_ip_src({10,0,0,45});
  orig_pckt->set_ip_dst({10,0,0,50});
  orig_pckt->set_ip_total_length(50);
  auto total_length = orig_pckt->ip_total_length();

  auto udp_pckt = static_unique_ptr_cast<PacketUDP>(std::move(orig_pckt));
  udp_pckt->set_src_port(80);
  udp_pckt->set_dst_port(80);
  Socket dest{udp_pckt->ip_dst(), udp_pckt->dst_port()};

  inet.ip_obj().update_path(dest, err.pmtu(), too_big, total_length);
  err.set_pmtu(inet.ip_obj().pmtu(dest));

  EXPECT(err.pmtu() == 68);
  EXPECT(inet.ip_obj().pmtu(dest) == 68);
}

CASE("Turning Path MTU Discovery off clears the PMTU cache")
{
  Nic_mock nic;
  Inet inet{nic};

  inet.set_path_mtu_discovery(true);

  ICMP_error err{icmp4::Type::DEST_UNREACHABLE, (uint8_t) icmp4::code::Dest_unreachable::FRAGMENTATION_NEEDED, 1400};
  bool too_big = err.is_too_big();
  Socket dest{ip4::Addr{10,0,0,48}, 443};

  inet.ip_obj().update_path(dest, err.pmtu(), too_big);

  EXPECT(inet.ip_obj().pmtu(dest) == 1400);

  inet.set_path_mtu_discovery(false); // Clears the PMTU cache and stops the timer if is running

  EXPECT(inet.ip_obj().pmtu(dest) == 0);
}

CASE("No PMTU entry exists for non-existing path")
{
  Nic_mock nic;
  Inet inet{nic};

  inet.set_path_mtu_discovery(true);

  EXPECT(inet.ip_obj().pmtu(Socket{ip4::Addr{10,0,0,45}, 80}) == 0);
}

CASE("A PMTU entry can be removed")
{
  Nic_mock nic;
  Inet inet{nic};

  inet.set_path_mtu_discovery(true);

  ICMP_error err{icmp4::Type::DEST_UNREACHABLE, (uint8_t) icmp4::code::Dest_unreachable::FRAGMENTATION_NEEDED, 1400};
  bool too_big = err.is_too_big();
  Socket dest{ip4::Addr{10,0,0,49}, 443};

  inet.ip_obj().update_path(dest, err.pmtu(), too_big);

  EXPECT(inet.ip_obj().pmtu(dest) == 1400);

  inet.ip_obj().remove_path(dest);

  EXPECT(inet.ip_obj().pmtu(dest) == 0);
}

CASE("The PMTU cache/map can be flushed")
{
  Nic_mock nic;
  Inet inet{nic};

  inet.set_path_mtu_discovery(true);

  ICMP_error err1{icmp4::Type::DEST_UNREACHABLE, (uint8_t) icmp4::code::Dest_unreachable::FRAGMENTATION_NEEDED, 1000};
  bool too_big1 = err1.is_too_big();
  Socket dest1{ip4::Addr{10,0,0,5}, 80};

  ICMP_error err2{icmp4::Type::DEST_UNREACHABLE, (uint8_t) icmp4::code::Dest_unreachable::FRAGMENTATION_NEEDED, 900};
  bool too_big2 = err2.is_too_big();
  Socket dest2{ip4::Addr{10,0,0,6}, 80};

  inet.ip_obj().update_path(dest1, err1.pmtu(), too_big1);
  inet.ip_obj().update_path(dest2, err2.pmtu(), too_big2);

  EXPECT(inet.ip_obj().pmtu(dest1) == 1000);
  EXPECT(inet.ip_obj().pmtu(dest2) == 900);

  inet.ip_obj().flush_paths();

  EXPECT(inet.ip_obj().pmtu(dest1) == 0);
  EXPECT(inet.ip_obj().pmtu(dest2) == 0);
}

/*
 *  Unit tests for when Path MTU Discovery is enabled by default:
 *
CASE("Path MTU Discovery is enabled by default and can be disabled")
{
  Nic_mock nic;
  Inet inet{nic};
  EXPECT(inet.ip_obj().path_mtu_discovery());

  inet.set_path_mtu_discovery(false);
  EXPECT(not inet.ip_obj().path_mtu_discovery());
}

CASE("An ICMP Too Big message with a next hop MTU value of 45 is invalid")
{
  Nic_mock nic;
  Inet inet{nic};

  ICMP_error err{icmp4::Type::DEST_UNREACHABLE, (uint8_t) icmp4::code::Dest_unreachable::FRAGMENTATION_NEEDED, 45};
  bool too_big = err.is_too_big();

  auto orig_pckt = inet.create_ip_packet(Protocol::UDP);
  orig_pckt->set_ip_src({10,0,0,45});
  orig_pckt->set_ip_dst({10,0,0,50});
  orig_pckt->set_ip_total_length(50);
  auto total_length = orig_pckt->ip_total_length();

  auto udp_pckt = static_unique_ptr_cast<PacketUDP>(std::move(orig_pckt));
  udp_pckt->set_src_port(80);
  udp_pckt->set_dst_port(80);
  Socket dest{udp_pckt->ip_dst(), udp_pckt->dst_port()};

  inet.ip_obj().update_path(dest, err.pmtu(), too_big, total_length);
  err.set_pmtu(inet.ip_obj().pmtu(dest));

  EXPECT(err.pmtu() == 0);  // Invalid PMTU value 45
}

CASE("An ICMP Too Big message with a next hop MTU value of 68 is valid")
{
  Nic_mock nic;
  Inet inet{nic};

  ICMP_error err{icmp4::Type::DEST_UNREACHABLE, (uint8_t) icmp4::code::Dest_unreachable::FRAGMENTATION_NEEDED, 68};
  bool too_big = err.is_too_big();

  auto orig_pckt = inet.create_ip_packet(Protocol::UDP);
  orig_pckt->set_ip_src({10,0,0,45});
  orig_pckt->set_ip_dst({10,0,0,50});
  orig_pckt->set_ip_total_length(50);
  auto total_length = orig_pckt->ip_total_length();

  auto udp_pckt = static_unique_ptr_cast<PacketUDP>(std::move(orig_pckt));
  udp_pckt->set_src_port(80);
  udp_pckt->set_dst_port(80);
  Socket dest{udp_pckt->ip_dst(), udp_pckt->dst_port()};

  inet.ip_obj().update_path(dest, err.pmtu(), too_big, total_length);
  err.set_pmtu(inet.ip_obj().pmtu(dest));

  EXPECT(err.pmtu() == 68);
  EXPECT(inet.ip_obj().pmtu(dest) == 68);
}

CASE("Turning Path MTU Discovery off clears the PMTU cache")
{
  Nic_mock nic;
  Inet inet{nic};

  ICMP_error err{icmp4::Type::DEST_UNREACHABLE, (uint8_t) icmp4::code::Dest_unreachable::FRAGMENTATION_NEEDED, 1400};
  bool too_big = err.is_too_big();
  Socket dest{{10,0,0,48}, 443};

  inet.ip_obj().update_path(dest, err.pmtu(), too_big);

  EXPECT(inet.ip_obj().pmtu(dest) == 1400);

  inet.set_path_mtu_discovery(false); // Clears the PMTU cache and stops the timer if is running

  EXPECT(inet.ip_obj().pmtu(dest) == 0);
}

CASE("No PMTU entry exists for non-existing path")
{
  Nic_mock nic;
  Inet inet{nic};

  EXPECT(inet.ip_obj().pmtu(Socket{{10,0,0,45}, 80}) == 0);
}

CASE("A PMTU entry can be removed")
{
  Nic_mock nic;
  Inet inet{nic};

  ICMP_error err{icmp4::Type::DEST_UNREACHABLE, (uint8_t) icmp4::code::Dest_unreachable::FRAGMENTATION_NEEDED, 1400};
  bool too_big = err.is_too_big();
  Socket dest{{10,0,0,49}, 443};

  inet.ip_obj().update_path(dest, err.pmtu(), too_big);

  EXPECT(inet.ip_obj().pmtu(dest) == 1400);

  inet.ip_obj().remove_path(dest);

  EXPECT(inet.ip_obj().pmtu(dest) == 0);
}

CASE("The PMTU cache/map can be flushed")
{
  Nic_mock nic;
  Inet inet{nic};

  ICMP_error err1{icmp4::Type::DEST_UNREACHABLE, (uint8_t) icmp4::code::Dest_unreachable::FRAGMENTATION_NEEDED, 1000};
  bool too_big1 = err1.is_too_big();
  Socket dest1{{10,0,0,5}, 80};

  ICMP_error err2{icmp4::Type::DEST_UNREACHABLE, (uint8_t) icmp4::code::Dest_unreachable::FRAGMENTATION_NEEDED, 900};
  bool too_big2 = err2.is_too_big();
  Socket dest2{{10,0,0,6}, 80};

  inet.ip_obj().update_path(dest1, err1.pmtu(), too_big1);
  inet.ip_obj().update_path(dest2, err2.pmtu(), too_big2);

  EXPECT(inet.ip_obj().pmtu(dest1) == 1000);
  EXPECT(inet.ip_obj().pmtu(dest2) == 900);

  inet.ip_obj().flush_paths();

  EXPECT(inet.ip_obj().pmtu(dest1) == 0);
  EXPECT(inet.ip_obj().pmtu(dest2) == 0);
}
*/
