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

#ifndef NET_DHCP_DHCP4_HPP
#define NET_DHCP_DHCP4_HPP

#include <net/ip4/udp.hpp>

namespace net {

  static const int DHCP_VEND_LEN =        304;
  static const int BOOTP_MIN_LEN =        300;
  static const int DHCP_MIN_LEN =         548;
  // some clients silently ignore responses less than 300 bytes
  static const int DEFAULT_PACKET_SIZE =  300;

  struct dhcp_option_t
  {
    uint8_t code;
    uint8_t length;
    uint8_t val[0];
  };

  struct dhcp_packet_t
  {
    static const uint8_t CHADDR_LEN =  16;
    static const uint8_t SNAME_LEN  =  64;
    static const uint8_t FILE_LEN   = 128;

    uint8_t   op;           // message opcode
    uint8_t   htype;        // hardware addr type
    uint8_t   hlen;         // hardware addr length
    uint8_t   hops;         // relay agent hops from client
    uint32_t  xid;          // transaction ID
    uint16_t  secs;         // seconds since start
    uint16_t  flags;        // flag bits
    IP4::addr ciaddr;       // client IP address
    IP4::addr yiaddr;       // client IP address
    IP4::addr siaddr;       // IP address of next server
    IP4::addr giaddr;       // DHCP relay agent IP address
    uint8_t   chaddr[CHADDR_LEN];  // client hardware address
    uint8_t   sname[SNAME_LEN];    // server name
    uint8_t   file[FILE_LEN];      // BOOT filename
    uint8_t   magic[4];            // option_format aka magic
    dhcp_option_t   options[0];
  };

  static const uint16_t PACKET_SIZE = sizeof(dhcp_packet_t) + DHCP_VEND_LEN;

  // BOOTP (rfc951) message types
  static const uint8_t BOOTREQUEST =  1;
  static const uint8_t BOOTREPLY =    2;

  // Possible values for flags field
  static const uint32_t BOOTP_UNICAST =   0x0000;
  static const uint32_t BOOTP_BROADCAST = 0x0080;

  // Possible values for hardware type (htype) field
  static const uint8_t HTYPE_ETHER =    1;  // Ethernet 10Mbps
  static const uint8_t HTYPE_IEEE802 =  6;  // IEEE 802.2 Token Ring
  static const uint8_t HTYPE_FDDI =     8;  // FDDI
  static const uint8_t HTYPE_HFI =      37; // HFI

  /* Magic cookie validating dhcp options field (and bootp vendor
     extensions field). */
  static const std::string DHCP_OPTIONS_COOKIE = "\143\202\123\143";

  // DHCP Option codes
  static const uint8_t DHO_PAD =                          0;
  static const uint8_t DHO_SUBNET_MASK =                  1;
  static const uint8_t DHO_TIME_OFFSET =                  2;
  static const uint8_t DHO_ROUTERS =                      3;
  static const uint8_t DHO_TIME_SERVERS =                 4;
  static const uint8_t DHO_NAME_SERVERS =                 5;
  static const uint8_t DHO_DOMAIN_NAME_SERVERS =          6;
  static const uint8_t DHO_LOG_SERVERS =                  7;
  static const uint8_t DHO_COOKIE_SERVERS =               8;
  static const uint8_t DHO_LPR_SERVERS =                  9;
  static const uint8_t DHO_IMPRESS_SERVERS =              10;
  static const uint8_t DHO_RESOURCE_LOCATION_SERVERS =    11;
  static const uint8_t DHO_HOST_NAME =                    12;
  static const uint8_t DHO_BOOT_SIZE =                    13;
  static const uint8_t DHO_MERIT_DUMP =                   14;
  static const uint8_t DHO_DOMAIN_NAME =                  15;
  static const uint8_t DHO_SWAP_SERVER =                  16;
  static const uint8_t DHO_ROOT_PATH =                    17;
  static const uint8_t DHO_EXTENSIONS_PATH =              18;
  static const uint8_t DHO_IP_FORWARDING =                19;
  static const uint8_t DHO_NON_LOCAL_SOURCE_ROUTING =     20;
  static const uint8_t DHO_POLICY_FILTER =                21;
  static const uint8_t DHO_MAX_DGRAM_REASSEMBLY =         22;
  static const uint8_t DHO_DEFAULT_IP_TTL =               23;
  static const uint8_t DHO_PATH_MTU_AGING_TIMEOUT =       24;
  static const uint8_t DHO_PATH_MTU_PLATEAU_TABLE =       25;
  static const uint8_t DHO_INTERFACE_MTU =                26;
  static const uint8_t DHO_ALL_SUBNETS_LOCAL =            27;
  static const uint8_t DHO_BROADCAST_ADDRESS =            28;
  static const uint8_t DHO_PERFORM_MASK_DISCOVERY =       29;
  static const uint8_t DHO_MASK_SUPPLIER =                30;
  static const uint8_t DHO_ROUTER_DISCOVERY =             31;
  static const uint8_t DHO_ROUTER_SOLICITATION_ADDRESS =  32;
  static const uint8_t DHO_STATIC_ROUTES =                33;
  static const uint8_t DHO_TRAILER_ENCAPSULATION =        34;
  static const uint8_t DHO_ARP_CACHE_TIMEOUT =            35;
  static const uint8_t DHO_IEEE802_3_ENCAPSULATION =      36;
  static const uint8_t DHO_DEFAULT_TCP_TTL =              37;
  static const uint8_t DHO_TCP_KEEPALIVE_INTERVAL =       38;
  static const uint8_t DHO_TCP_KEEPALIVE_GARBAGE =        39;
  static const uint8_t DHO_NIS_DOMAIN =                   40;
  static const uint8_t DHO_NIS_SERVERS =                  41;
  static const uint8_t DHO_NTP_SERVERS =                  42;
  static const uint8_t DHO_VENDOR_ENCAPSULATED_OPTIONS =  43;
  static const uint8_t DHO_NETBIOS_NAME_SERVERS =         44;
  static const uint8_t DHO_NETBIOS_DD_SERVER =            45;
  static const uint8_t DHO_NETBIOS_NODE_TYPE =            46;
  static const uint8_t DHO_NETBIOS_SCOPE =                47;
  static const uint8_t DHO_FONT_SERVERS =                 48;
  static const uint8_t DHO_X_DISPLAY_MANAGER =            49;
  static const uint8_t DHO_DHCP_REQUESTED_ADDRESS =       50;
  static const uint8_t DHO_DHCP_LEASE_TIME =              51;
  static const uint8_t DHO_DHCP_OPTION_OVERLOAD =         52;
  static const uint8_t DHO_DHCP_MESSAGE_TYPE =            53;
  static const uint8_t DHO_DHCP_SERVER_IDENTIFIER =       54;
  static const uint8_t DHO_DHCP_PARAMETER_REQUEST_LIST =  55;
  static const uint8_t DHO_DHCP_MESSAGE =                 56;
  static const uint8_t DHO_DHCP_MAX_MESSAGE_SIZE =        57;
  static const uint8_t DHO_DHCP_RENEWAL_TIME =            58;
  static const uint8_t DHO_DHCP_REBINDING_TIME =          59;
  static const uint8_t DHO_VENDOR_CLASS_IDENTIFIER =      60;
  static const uint8_t DHO_DHCP_CLIENT_IDENTIFIER =       61;
  static const uint8_t DHO_NWIP_DOMAIN_NAME =             62;
  static const uint8_t DHO_NWIP_SUBOPTIONS =              63;
  static const uint8_t DHO_USER_CLASS =                   77;
  static const uint8_t DHO_FQDN =                         81;
  static const uint8_t DHO_DHCP_AGENT_OPTIONS =           82;
  static const uint8_t DHO_SUBNET_SELECTION =             118;  // RFC3011
  /* The DHO_AUTHENTICATE option is not a standard yet, so I've
     allocated an option out of the "local" option space for it on a
     temporary basis.  Once an option code number is assigned, I will
     immediately and shamelessly break this, so don't count on it
     continuing to work. */
  static const uint8_t DHO_AUTHENTICATE = 210;
  static const uint8_t DHO_END =          255;

  // DHCP message types
  static const uint8_t DHCPDISCOVER = 1;
  static const uint8_t DHCPOFFER =    2;
  static const uint8_t DHCPREQUEST =  3;
  static const uint8_t DHCPDECLINE =  4;
  static const uint8_t DHCPACK =      5;
  static const uint8_t DHCPNAK =      6;
  static const uint8_t DHCPRELEASE =  7;
  static const uint8_t DHCPINFORM =   8;

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

  static inline dhcp_option_t* conv_option(dhcp_option_t* option, int offset)
  {
    return (dhcp_option_t*) ((char*) option + offset);
  }

  static inline const dhcp_option_t* get_option(const dhcp_option_t* opts, uint8_t code)
  {
    const dhcp_option_t* opt = opts;
    while (opt->code != code && opt->code != DHO_END)
    {
      // go to next option
      opt = (const dhcp_option_t*) (((const uint8_t*) opt) + 2 + opt->length);
    }
    return opt;
  }

} // < namespace net

#endif
