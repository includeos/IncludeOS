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

namespace net::dns {

  struct Query {
    const id_t        id;
    const std::string hostname;
    const Record_type rtype;

    Query(std::string hostname, const Record_type rtype)
      : Query{generate_id(), std::move(hostname), rtype}
    {}

    Query(const id_t id, std::string hostname, const Record_type rtype)
      : id{id},
        hostname{std::move(hostname)},
        rtype{rtype}
    {}

    size_t write(char* buffer) const;

  private:
    // TODO: for now, needs to be randomized
    static unsigned short generate_id()
    {
      static unsigned short id = 0;
      return ++id;
    }

    int write_formatted_hostname(char* qname) const
    { return dns::encode_name(this->hostname, qname); }
  };

}
