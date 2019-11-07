
#pragma once
#ifndef PACKET_MLD_HPP
#define PACKET_MLD_HPP

#include <net/ip6/ip6.hpp>
#include <net/ip6/icmp6_error.hpp>
#include <cstdint>
#include <gsl/span>

namespace net::icmp6 {
  class Packet;
}

namespace net::mld {

  class MldPacket2 {
  private:
    icmp6::Packet&  icmp6_;

  public:
    struct Query {
      ip6::Addr multicast;
      uint8_t   reserved : 4,
                supress  : 1,
                qrc      : 3;
      uint8_t   qqic;
      uint16_t  num_srcs;
      ip6::Addr sources[0];
    };

    struct multicast_address_rec {
      uint8_t   rec_type;
      uint8_t   data_len;
      uint16_t  num_src;
      ip6::Addr multicast;
      ip6::Addr sources[0];
    };

    struct Report {
      multicast_address_rec records[0];
    };

    MldPacket2(icmp6::Packet& icmp6);
    Query& query();
    Report& report();
  };
}
#endif
