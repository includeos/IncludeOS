
#pragma once

#include "dns.hpp"
#include <net/addr.hpp>

namespace net::dns {

  struct Record
  {
    std::string name;
    Record_type rtype;
    Class       rclass;
    uint32_t    ttl;
    uint16_t    data_len;
    std::string rdata;

    Record() = default;

    int parse(const char* reader, const char* buffer, size_t len);
    void populate(const rr_data& res);
    int parse_name(const char* reader,
                   const char* buffer, size_t tot_len,
                   std::string& output) const;

    ip4::Addr get_ipv4() const;
    ip6::Addr get_ipv6() const;
    net::Addr get_addr() const;

    bool is_addr() const
    { return rtype == Record_type::A or rtype == Record_type::AAAA; }
  };
}
