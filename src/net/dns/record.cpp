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

#include <net/dns/record.hpp>

namespace net::dns {

  int Record::parse(const char* reader, const char* buffer)
  {
    int count = 0;

    const auto namelen = parse_name(reader, buffer, this->name);
    reader += namelen;
    count += namelen;

    const auto& resource = *(const rr_data*) reader;
    populate(resource);
    reader += sizeof(rr_data);
    count += sizeof(rr_data);

    switch(this->rtype)
    {
      case Record_type::A:    // IPv4
      case Record_type::AAAA: // IPv6
      {
        this->rdata.assign(reader, this->data_len);
        count += data_len;
        break;
      }
      default:
      {
        count += parse_name(reader, buffer, this->rdata);
      }
    }

    if(rtype == Record_type::AAAA)
      printf("IP6: %s\n", ((ip6::Addr*)rdata.data())->to_string().c_str());

    return count;
  }

  void Record::populate(const rr_data& res)
  {
    this->rtype     = static_cast<Record_type>(ntohs(res.type));
    this->rclass    = static_cast<Class>(ntohs(res._class));
    this->ttl       = ntohl(res.ttl);
    this->data_len  = ntohs(res.data_len);
  }

  int Record::parse_name(const char* reader, const char* buffer, std::string& output) const
  {
    Expects(output.empty());

    unsigned p = 0;
    unsigned offset = 0;
    bool jumped = false;

    int count = 1;
    unsigned char* ureader = (unsigned char*) reader;

    while (*ureader)
    {
      if (*ureader >= 192)
      {
        offset = (*ureader) * 256 + *(ureader+1) - 49152; // = 11000000 00000000
        ureader = (unsigned char*) buffer + offset - 1;
        jumped = true; // we have jumped to another location so counting wont go up!
      }
      else
      {
        output[p++] = *ureader;
      }
      ureader++;

      // if we havent jumped to another location then we can count up
      if (jumped == false) count++;
    }
    output.resize(p);

    // number of steps we actually moved forward in the packet
    if (jumped)
      count++;

    // now convert 3www6google3com0 to www.google.com
    int len = p; // same as name.size()
    int i;
    for(i = 0; i < len; i++)
    {
      p = output[i];

      for(unsigned j = 0; j < p; j++)
      {
        output[i] = output[i+1];
        i++;
      }
      output[i] = '.';
    }
    output[i - 1] = '\0'; // remove the last dot
    return count;
  }

  ip4::Addr Record::get_ipv4() const
  {
    Expects(rtype == Record_type::A);
    return *(ip4::Addr*) rdata.data();
  }

  ip6::Addr Record::get_ipv6() const
  {
    Expects(rtype == Record_type::AAAA);
    return *(ip6::Addr*) rdata.data();
  }

  net::Addr Record::get_addr() const
  {
    switch(rtype)
    {
      case Record_type::A:
        return get_ipv4();
      case Record_type::AAAA:
        return get_ipv6();
      default:
        return {};
    }
  }

}
