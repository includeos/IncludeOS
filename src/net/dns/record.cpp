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

  int Record::parse(const char* reader, const char* buffer, size_t len)
  {
    // don't call parse_name if we are already out of buffer
    if (reader >= buffer + len)
        throw std::runtime_error("Nothing left to parse");

    int count = 0;

    const auto namelen = parse_name(reader, buffer, len, this->name);
    count += namelen;
    assert(count >= 0);

    reader += namelen;
    const int remaining = len - (reader - buffer);
    assert(remaining <= (int) len);

    // invalid request if there is no room for resources
    if (remaining < (int) sizeof(rr_data))
      throw std::runtime_error("Nothing left to parse");

    // extract resource data header
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
        count += parse_name(reader, buffer, len, this->rdata);
      }
    }

    return count;
  }

  void Record::populate(const rr_data& res)
  {
    this->rtype     = static_cast<Record_type>(ntohs(res.type));
    this->rclass    = static_cast<Class>(ntohs(res._class));
    this->ttl       = ntohl(res.ttl);
    this->data_len  = ntohs(res.data_len);
  }

  int Record::parse_name(const char* reader,
                         const char* buffer, size_t tot_len,
                         std::string& output) const
  {
    Expects(output.empty());

    unsigned namelen = 0;
    bool jumped = false;

    int count = 1;
    const auto* ureader = (unsigned char*) reader;

    while (*ureader)
    {
      if (*ureader >= 192)
      {
        // read 16-bit offset, mask out the 2 top bits
        uint16_t offset = (*ureader >> 8) | *(ureader+1);
        offset &= 0x3fff; // remove 2 top bits

        if(UNLIKELY(offset > tot_len))
          return 0;

        ureader = (unsigned char*) &buffer[offset];
        jumped = true; // we have jumped to another location so counting wont go up!
      }
      else
      {
        output[namelen++] = *ureader++;

        // maximum label size
        if(UNLIKELY(namelen > 63)) break;
      }

      // if we havent jumped to another location then we can count up
      if (jumped == false) count++;
    }

    output.resize(namelen);
    if (output.empty()) return 0;

    // number of steps we actually moved forward in the packet
    if (jumped)
      count++;

    // now convert 3www6google3com0 to www.google.com
    for(unsigned i = 0; i < output.size(); i++)
    {
      const uint8_t len = output[i];

      for(unsigned j = 0; j < len; j++)
      {
        output[i] = output[i+1];
        i++;
      }
      output[i] = '.';
    }
    // remove the last dot by resizing down
    output.resize(output.size()-1);
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
