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
#ifndef PACKET_NDP_HPP
#define PACKET_NDP_HPP

#include <net/ip6/ip6.hpp>
#include <net/ip6/icmp6_error.hpp>
#include <cstdint>
#include <gsl/span>

namespace net::icmp6 {
  class Packet;
}

namespace net::ndp {

static const int NEIGH_ADV_ROUTER   = 0x1;
static const int NEIGH_ADV_SOL      = 0x2;
static const int NEIGH_ADV_OVERRIDE = 0x4;

  enum {
      ND_OPT_PREFIX_INFO_END = 0,
      ND_OPT_SOURCE_LL_ADDR = 1, /* RFC2461 */
      ND_OPT_TARGET_LL_ADDR = 2, /* RFC2461 */
      ND_OPT_PREFIX_INFO = 3,    /* RFC2461 */
      ND_OPT_REDIRECT_HDR = 4,   /* RFC2461 */
      ND_OPT_MTU = 5,            /* RFC2461 */
      ND_OPT_NONCE = 14,         /* RFC7527 */
      ND_OPT_ARRAY_MAX,
      ND_OPT_ROUTE_INFO = 24,    /* RFC4191 */
      ND_OPT_RDNSS = 25,         /* RFC5006 */
      ND_OPT_DNSSL = 31,         /* RFC6106 */
      ND_OPT_6CO = 34,           /* RFC6775 */
      ND_OPT_MAX
  };

  struct nd_options_header {
      uint8_t type;
      uint8_t len;
      uint8_t payload[0];
  } __attribute__((packed));

  class NdpOptions {
  private:
    struct route_info {
      uint8_t  type;
      uint8_t  len;
      uint8_t  prefix_len;
      uint8_t  reserved_l:3,
               route_pref:2,
               reserved_h:2;
      uint32_t lifetime;
      uint8_t  prefix[0];
    };

    struct prefix_info {
      uint8_t   type;
      uint8_t   len;
      uint8_t   prefix_len;
      uint8_t   onlink:1,
                autoconf:1,
                reserved:6;
      uint32_t  valid;
      uint32_t  prefered;
      uint32_t  reserved2;
      ip6::Addr prefix;
    };

    struct nd_options_header *header_;
    struct nd_options_header *nd_opts_ri;
    struct nd_options_header *nd_opts_ri_end;
    struct nd_options_header *user_opts;
    struct nd_options_header *user_opts_end;
    std::array<struct nd_options_header*, ND_OPT_ARRAY_MAX> opt_array;

    bool is_useropt(struct nd_options_header *opt)
    {
      if (opt->type == ND_OPT_RDNSS ||
        opt->type == ND_OPT_DNSSL) {
        return true;
      }
      return false;
    }

    struct nd_options_header *next_option(struct nd_options_header *cur,
            struct nd_options_header *end)
    {
      int type;

      if (!cur || !end || cur >= end)
          return nullptr;

      type = cur->type;

      do {
        cur += (cur->len << 3);
      } while (cur < end && cur->type != type);
      return cur <= end && cur->type == type ? cur : nullptr;
    }

    prefix_info* pinfo_next(prefix_info* cur)
    {
      return reinterpret_cast<prefix_info *> (
             next_option(reinterpret_cast<nd_options_header *>(cur),
             opt_array[ND_OPT_PREFIX_INFO_END]));
    }

    route_info* rinfo_next(route_info* cur)
    {
      return reinterpret_cast<route_info*> (
             next_option(reinterpret_cast<nd_options_header *>(cur),
             nd_opts_ri_end));
    }

  public:
    using Pinfo_handler = delegate<void(ip6::Addr, uint32_t, uint32_t)>;

    NdpOptions() : header_{nullptr}, nd_opts_ri{nullptr},
        nd_opts_ri_end{nullptr}, user_opts{nullptr},
        user_opts_end{nullptr}, opt_array{} {}

    void parse(uint8_t *opt, uint16_t opts_len);
    bool parse_prefix(Pinfo_handler autoconf_cb,
        Pinfo_handler onlink_cb);

    struct nd_options_header *get_header(uint8_t &opt)
    {
      return reinterpret_cast<struct nd_options_header*>(opt);
    }

    uint8_t *get_option_data(uint8_t option)
    {
      if (option < ND_OPT_ARRAY_MAX) {
          if (opt_array[option]) {
              return static_cast<uint8_t*> (opt_array[option]->payload);
          }
      }
      return nullptr;
    }

    struct nd_options_header *option(uint8_t option)
    {
      if (option < ND_OPT_ARRAY_MAX) {
          if (opt_array[option]) {
              return opt_array[option];
          }
      } else if (option == ND_OPT_ROUTE_INFO) {
          return nd_opts_ri;
      } else if (option == ND_OPT_RDNSS ||
          option == ND_OPT_DNSSL ) {
      }
      return nullptr;
    }
  };

  class NdpPacket {

    using Pinfo_handler = NdpOptions::Pinfo_handler;
    using ICMP_type = ICMP6_error::ICMP_type;
  private:

    struct RouterSol
    {
      uint8_t  options[0];

      uint16_t option_offset()
      { return 0; }

    } __attribute__((packed));

    struct RouterAdv
    {
      uint32_t reachable_time_;
      uint32_t retrans_time_;
      uint8_t  options[0];

      uint32_t reachable_time()
      { return reachable_time_; }

      uint32_t retrans_time()
      { return retrans_time_; }

      uint16_t option_offset()
      { return 8; }

    } __attribute__((packed));

    struct RouterRedirect
    {
      ip6::Addr target_;
      ip6::Addr dest_;
      uint8_t  options[0];

      ip6::Addr target()
      { return target_; }

      ip6::Addr dest()
      { return dest_; }

      uint16_t option_offset()
      { return IP6_ADDR_BYTES * 2; }

    } __attribute__((packed));

    struct NeighborSol
    {
      ip6::Addr target_;
      uint8_t   options[0];

      ip6::Addr target()
      { return target_; }

      uint16_t option_offset()
      { return IP6_ADDR_BYTES; }

    } __attribute__((packed));

    struct NeighborAdv
    {
      ip6::Addr target_;
      uint8_t   options[0];

      ip6::Addr target()
      { return target_; }

      uint16_t option_offset()
      { return IP6_ADDR_BYTES; }

    } __attribute__((packed));

    icmp6::Packet&  icmp6_;
    NdpOptions      ndp_opt_;

  public:
    NdpPacket(icmp6::Packet& icmp6);

    void parse_options(icmp6::Type type);
    bool parse_prefix(Pinfo_handler autoconf_cb,
      Pinfo_handler onlink_cb);

    RouterSol& router_sol();
    RouterAdv& router_adv();
    RouterRedirect& router_redirect();
    NeighborSol& neighbour_sol();
    NeighborAdv& neighbour_adv();
    bool is_flag_router();
    bool is_flag_solicited();
    bool is_flag_override();
    void set_neighbour_adv_flag(uint32_t flag);
    void set_ndp_options_header(uint8_t type, uint8_t len);
    uint8_t* get_option_data(int opt)
    {
        return ndp_opt_.get_option_data(opt);
    }
  };
}
#endif
