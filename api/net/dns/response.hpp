
#pragma once

#include "dns.hpp"
#include "record.hpp"
#include <vector>

namespace net::dns {

  class Response {
  public:
    Response() = default;
    Response(const char* buffer, size_t len)
    {
      parse(buffer, len);
    }

    std::vector<Record> answers;
    std::vector<Record> auth;
    std::vector<Record> addit;

    ip4::Addr get_first_ipv4() const;
    ip6::Addr get_first_ipv6() const;
    net::Addr get_first_addr() const;

    bool has_addr() const;

    int parse(const char* buffer, size_t len);
  };

}
