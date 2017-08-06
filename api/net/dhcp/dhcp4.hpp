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

#pragma once
#ifndef NET_DHCP_DHCP4_HPP
#define NET_DHCP_DHCP4_HPP

#include <cstdint>
#include <string>

namespace net {
namespace dhcp {

  static const int DHCP_VEND_LEN =        304;
  static const int BOOTP_MIN_LEN =        300;
  static const int DHCP_MIN_LEN =         548;
  // some clients silently ignore responses less than 300 bytes
  static const int DEFAULT_PACKET_SIZE =  300;

  // BOOTP (rfc951) message types
  enum class op_code : uint8_t
  {
    BOOTREQUEST = 1,
    BOOTREPLY   = 2
  };

  // Possible values for flags field
  enum class flag : uint16_t
  {
    BOOTP_UNICAST   = 0x0000,
    BOOTP_BROADCAST = 0x8000,
  };

  // Possible values for hardware type (htype) field
  enum class htype : uint8_t
  {
    ETHER =    1,  // Ethernet 10Mbps
    IEEE802 =  6,  // IEEE 802.2 Token Ring
    FDDI =     8,  // FDDI
    HFI =      37  // HFI
  };

  /* Magic cookie validating dhcp options field (and bootp vendor
     extensions field). */
  static const std::string DHCP_OPTIONS_COOKIE = "\143\202\123\143";

  // DHCP message types
  enum class message_type : uint8_t
  {
    DISCOVER = 1,
    OFFER =    2,
    REQUEST =  3,
    DECLINE =  4,
    ACK =      5,
    NAK =      6,
    RELEASE =  7,
    INFORM =   8
  };

  // Relay Agent Information option subtypes
  static const uint8_t RAI_CIRCUIT_ID = 1;
  static const uint8_t RAI_REMOTE_ID =  2;
  static const uint8_t RAI_AGENT_ID =   3;

  // FQDN suboptions
  static const uint8_t FQDN_NO_CLIENT_UPDATE =  1;
  static const uint8_t FQDN_SERVER_UPDATE =     2;
  static const uint8_t FQDN_ENCODED =           3;
  static const uint8_t FQDN_RCODE1 =            4;
  static const uint8_t FQDN_RCODE2 =            5;
  static const uint8_t FQDN_HOSTNAME =          6;
  static const uint8_t FQDN_DOMAINNAME =        7;
  static const uint8_t FQDN_FQDN =              8;
  static const uint8_t FQDN_SUBOPTION_COUNT =   8;

  static const uint8_t ETH_ALEN =         6;  // octets in one ethernet header
  static const uint8_t DHCP_SERVER_PORT = 67;
  static const uint8_t DHCP_CLIENT_PORT = 68;

} // < namespace dhcp
} // < namespace net

#endif
