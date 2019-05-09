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
