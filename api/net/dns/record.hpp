// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2018 IncludeOS AS, Oslo, Norway
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
