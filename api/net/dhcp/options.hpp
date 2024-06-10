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

#pragma once
#ifndef NET_DHCP_OPTIONS_HPP
#define NET_DHCP_OPTIONS_HPP

#include "dhcp4.hpp"
#include <vector>
#include <net/util.hpp>
#include <cstring>
#include <expects>

namespace net {
namespace dhcp {
namespace option {

enum Code : uint8_t
{
  PAD =                          0,
  SUBNET_MASK =                  1,
  TIME_OFFSET =                  2,
  ROUTERS =                      3,
  TIME_SERVERS =                 4,
  NAME_SERVERS =                 5,
  DOMAIN_NAME_SERVERS =          6,
  LOG_SERVERS =                  7,
  COOKIE_SERVERS =               8,
  LPR_SERVERS =                  9,
  IMPRESS_SERVERS =              10,
  RESOURCE_LOCATION_SERVERS =    11,
  HOST_NAME =                    12,
  BOOT_SIZE =                    13,
  MERIT_DUMP =                   14,
  DOMAIN_NAME =                  15,
  SWAP_SERVER =                  16,
  ROOT_PATH =                    17,
  EXTENSIONS_PATH =              18,
  IP_FORWARDING =                19,
  NON_LOCAL_SOURCE_ROUTING =     20,
  POLICY_FILTER =                21,
  MAX_DGRAM_REASSEMBLY =         22,
  DEFAULT_IP_TTL =               23,
  PATH_MTU_AGING_TIMEOUT =       24,
  PATH_MTU_PLATEAU_TABLE =       25,
  INTERFACE_MTU =                26,
  ALL_SUBNETS_LOCAL =            27,
  BROADCAST_ADDRESS =            28,
  PERFORM_MASK_DISCOVERY =       29,
  MASK_SUPPLIER =                30,
  ROUTER_DISCOVERY =             31,
  ROUTER_SOLICITATION_ADDRESS =  32,
  STATIC_ROUTES =                33,
  TRAILER_ENCAPSULATION =        34,
  ARP_CACHE_TIMEOUT =            35,
  IEEE802_3_ENCAPSULATION =      36,
  DEFAULT_TCP_TTL =              37,
  TCP_KEEPALIVE_INTERVAL =       38,
  TCP_KEEPALIVE_GARBAGE =        39,
  NIS_DOMAIN =                   40,
  NIS_SERVERS =                  41,
  NTP_SERVERS =                  42,
  VENDOR_ENCAPSULATED_OPTIONS =  43,
  NETBIOS_NAME_SERVERS =         44,
  NETBIOS_DD_SERVER =            45,
  NETBIOS_NODE_TYPE =            46,
  NETBIOS_SCOPE =                47,
  FONT_SERVERS =                 48,
  X_DISPLAY_MANAGER =            49,
  DHCP_REQUESTED_ADDRESS =       50,
  DHCP_LEASE_TIME =              51,
  DHCP_OPTION_OVERLOAD =         52,
  DHCP_MESSAGE_TYPE =            53,
  DHCP_SERVER_IDENTIFIER =       54,
  DHCP_PARAMETER_REQUEST_LIST =  55,
  DHCP_MESSAGE =                 56,
  DHCP_MAX_MESSAGE_SIZE =        57,
  DHCP_RENEWAL_TIME =            58,
  DHCP_REBINDING_TIME =          59,
  VENDOR_CLASS_IDENTIFIER =      60,
  DHCP_CLIENT_IDENTIFIER =       61,
  NWIP_DOMAIN_NAME =             62,
  NWIP_SUBOPTIONS =              63,
  USER_CLASS =                   77,
  FQDN =                         81,
  DHCP_AGENT_OPTIONS =           82,
  SUBNET_SELECTION =             118,
  AUTHENTICATE =                 210,
  END =                          255
};

/**
 * @brief      General option base. Needs to be inherited by a DHCP option.
 */
struct base
{
  const Code    code    {PAD};
  const uint8_t length  {0};
  uint8_t       val[0];

  constexpr base(const Code c, const uint8_t len) noexcept
    : code{c}, length{len}, val{}
  {}

  /**
   * @brief      Returns the total size of the option (included 2 bytes for code & length)
   *
   * @return     Total size of the option in bytes
   */
  constexpr uint8_t size() const noexcept
  { return length + sizeof(code) + sizeof(length); }

} __attribute__((packed));

/**
 * @brief      Determines what type of option it is. Used for "reflection".
 *
 * @tparam     C     option code
 */
template <Code C> struct type
{ static constexpr Code CODE{C}; };

/**
 * @brief      General One Address option. Works for IP and MAC
 */
struct addr_option : public base
{
  template <typename Addr>
  constexpr addr_option(const Code c, const Addr* addr) noexcept
    : base{c, sizeof(Addr)}
  {
    memcpy(&val[0], addr, sizeof(Addr));
  }

  template <typename Addr>
  const Addr* addr() const noexcept
  { return reinterpret_cast<const Addr*>(&val[0]); }
};

/**
 * @brief      END (255)
 */
struct end : public type<END>, public base
{
  constexpr end() noexcept
    : base{END, 0}
  {}
};

/**
 * @brief      SUBNET_MASK (1)
 */
struct subnet_mask : public type<SUBNET_MASK>, public addr_option
{
  template <typename Addr>
  constexpr subnet_mask(const Addr* addr) noexcept
    : addr_option{SUBNET_MASK, addr}
  {
  }
};

/**
 * @brief      TIME_OFFSET (2)
 */
struct time_offset : public type<TIME_OFFSET>, public base
{
  constexpr time_offset(int32_t offset_secs) noexcept
    : base{type::CODE, 4}
  {
    *(reinterpret_cast<int32_t*>(&val[0])) = htonl(offset_secs);
  }
};

/**
 * @brief      ROUTERS (3)
 */
struct routers : public type<ROUTERS>, public addr_option
{
  template <typename Addr>
  constexpr routers(const Addr* addr) noexcept
    : addr_option{CODE, addr}
  {
  }
};

// TODO: This currently only supports *one* address, need to support more than one
/**
 * @brief      DOMAIN_NAME_SERVERS (6)
 */
struct domain_name_servers : public type<DOMAIN_NAME_SERVERS>, public addr_option
{
  template <typename Addr>
  constexpr domain_name_servers(const Addr* addr) noexcept
    : addr_option{CODE, addr}
  {
  }
};

/**
 * @brief      DOMAIN_NAME (15)
 */
struct domain_name : public type<DOMAIN_NAME>, public base
{
  domain_name(const std::string& dname) noexcept
    : base{type::CODE, static_cast<uint8_t>(dname.size())}
  {
    std::memcpy(&val[0], dname.data(), dname.size());
  }

  std::string name() const
  { return {reinterpret_cast<const char*>(&val[0]), length}; }
};

/**
 * @brief      BROADCAST_ADDRESS (28)
 */
struct broadcast_address : public type<BROADCAST_ADDRESS>, public addr_option
{
  template <typename Addr>
  constexpr broadcast_address(const Addr* addr) noexcept
    : addr_option{CODE, addr}
  {
  }
};

/**
 * @brief      DHCP_REQUESTED_ADDRESS (50)
 */
struct req_addr : public type<DHCP_REQUESTED_ADDRESS>, public addr_option
{
  template <typename Addr>
  constexpr req_addr(const Addr* addr) noexcept
    : addr_option{DHCP_REQUESTED_ADDRESS, addr}
  {
  }
};

/**
 * @brief      DHCP_LEASE_TIME (51)
 */
struct lease_time : public type<DHCP_LEASE_TIME>, public base
{
  constexpr lease_time(uint32_t secs) noexcept
    : base{type::CODE, 4}
  {
    *(reinterpret_cast<uint32_t*>(&val[0])) = htonl(secs);
  }

  uint32_t secs() const noexcept
  { return ntohl(*(reinterpret_cast<const uint32_t*>(&val[0]))); }
};

/**
 * @brief      DHCP_MESSAGE_TYPE (53)
 */
struct message_type : public type<DHCP_MESSAGE_TYPE>, public base
{
  constexpr message_type(const dhcp::message_type m_type) noexcept
    : base{DHCP_MESSAGE_TYPE, 1}
  {
    val[0] = static_cast<uint8_t>(m_type);
  }

  dhcp::message_type type() const noexcept
  { return static_cast<dhcp::message_type>(val[0]); }
};

/**
 * @brief      DHCP_SERVER_IDENTIFIER (54)
 */
struct server_identifier : public type<DHCP_SERVER_IDENTIFIER>, public addr_option
{
  template <typename Addr>
  constexpr server_identifier(const Addr* addr) noexcept
    : addr_option{CODE, addr}
  {
  }
};

/**
 * @brief      DHCP_PARAMETER_REQUEST_LIST (55)
 */
struct param_req_list : public type<DHCP_PARAMETER_REQUEST_LIST>, public base
{
  param_req_list(const std::vector<Code>& codes) noexcept
    : base{DHCP_PARAMETER_REQUEST_LIST, static_cast<uint8_t>(codes.size())}
  {
    Expects(codes.size() < 50); // or something
    std::memcpy(&val[0], codes.data(), codes.size());
  }
};

/**
 * @brief      DHCP_MESSAGE (56)
 */
struct message : public type<DHCP_MESSAGE>, public base
{
  message(const std::string& msg) noexcept
    : base{CODE, static_cast<uint8_t>(msg.size())}
  {
    Expects(not msg.empty() and msg.size() <= 128); // or something
    std::memcpy(&val[0], msg.data(), msg.size());
  }

  std::string msg() const
  { return {reinterpret_cast<const char*>(&val[0]), length}; }
};

/**
 * @brief      DHCP_CLIENT_IDENTIFIER (61)
 */
struct client_identifier : public type<DHCP_CLIENT_IDENTIFIER>, public base
{
  template <typename Addr>
  constexpr client_identifier(const htype type, const Addr* addr) noexcept
    : base{DHCP_CLIENT_IDENTIFIER, sizeof(type) + sizeof(Addr)}
  {
    val[0] = static_cast<uint8_t>(type);
    memcpy(&val[1], addr, sizeof(Addr));
  }

  template <typename Addr>
  const Addr* addr() const noexcept
  { return reinterpret_cast<const Addr*>(&val[1]); }
};

} // < namespace option
} // < namespace dhcp
} // < namespace net

#endif
