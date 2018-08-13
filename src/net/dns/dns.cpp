// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015 Oslo and Akershus University College of Applied Sciences
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

//#define DNS_DEBUG 1
#ifdef DNS_DEBUG
#define PRINT(fmt, ...) printf(fmt, ##__VA_ARGS__)
#else
#define PRINT(fmt, ...) /* fmt */
#endif
#include <common>
#include <net/dns/dns.hpp>
#include <net/util.hpp>

#include <string>

using namespace std;

namespace net::dns
{

  int encode_name(std::string name, char* dst)
  {
    int lock = 0;

    name.push_back('.');
    int len = name.size();

    for(int i = 0; i < len; i++)
    {
      if (name[i] == '.')
      {
        *dst++ = i - lock;
        for(; lock < i; lock++)
        {
          *dst++ = name[lock];
        }
        lock++;
      }
    }
    *dst++ = '\0';

    return len + 1;
  }

  inline std::string parse_dns_query(unsigned char* c)
  {
    auto tmp = c;
    std::string resp;

    while (*(tmp)!=0)
      {
        int len = *tmp++;
        resp.append((char*) tmp, len);
        resp.append(".");
        tmp += len;
      }
    return resp;
  }

  int create_response(Header& hdr, lookup_func lookup)
  {
    PRINT("Request ID: %i \n", htons(hdr.id));
    PRINT("\t Type: %s \n", (hdr.qr ? "RESPONSE" : "QUERY"));

#ifdef DNS_DEBUG
    unsigned short qno = ntohs(hdr.q_count);
    PRINT("Questions: %i \n ", qno);
#endif

    char* buffer = (char*) &hdr + sizeof(Header);

    /// NOTE: ASSUMING 1 QUESTION ///
    char* query = buffer;

    std::string parsed_query = parse_dns_query((unsigned char*) query);
    PRINT("Question: %s\n", parsed_query.c_str());

    buffer += parsed_query.size() + 1; // zero-terminated

#ifdef DNS_DEBUG
    question& q = *(question*) buffer;
    unsigned short qtype  = ntohs(q.qtype);
    unsigned short qclass = ntohs(q.qclass);

    PRINT("Type:  %s (%i)",DNS::question_string(qtype).c_str(), qtype);
    PRINT("\t Class: %s (%i)",((qclass == 1) ? "INET" : "Unknown class"),qclass);
#endif

    // go to next question (assuming 1 question!!!!)
    buffer += sizeof(Question);

    //////////////////////////
    /// RESPONSE PART HERE ///
    //////////////////////////

    // initial response size
    unsigned short packetlen = sizeof(Header) +
      sizeof(Question) + parsed_query.size() + 1;

    // set DNS QR to RESPONSE
    hdr.qr = DNS_QR_RESPONSE;
    hdr.aa = 1; // authoritah

    // auth & additional = 0, for now
    hdr.auth_count = 0;
    hdr.add_count  = 0;

    std::vector<ip4::Addr>* addrs = lookup(parsed_query);
    if (addrs == nullptr)
      {
        // not found
        PRINT("*** Could not find: %s", parsed_query.c_str());
        hdr.ans_count = 0;
        hdr.rcode     = static_cast<uint8_t>(Response_code::NO_ERROR);
      }
    else
      {
        PRINT("*** Found %lu results for %s", addrs->size(), parsed_query.c_str());
        // append answers
        for (auto addr : *addrs)
          {
            PRINT("*** Result: %s", addr.str().c_str());
            // add query
            int qlen = parsed_query.size() + 1;
            memcpy(buffer, query, qlen);
            buffer += qlen;
            packetlen += qlen; // (!)

            // add resource record
            rr_data* data = (rr_data*) buffer;

            data->type     = htons(DNS_TYPE_A);
            data->_class   = htons(DNS_CLASS_INET);
            data->ttl      = htons(0x7FFF); // just because
            data->data_len = htons(sizeof(ip4::Addr));
            buffer += sizeof(rr_data);

            // add resource itself
            *((ip4::Addr*) buffer) = addr; // IPv4 address
            buffer += sizeof(ip4::Addr);

            packetlen += sizeof(rr_data) + sizeof(ip4::Addr); // (!)
          } // addr

        // set dns header answer count (!)
        hdr.ans_count = htons((addrs->size() & 0xFFFF));
        hdr.rcode     = static_cast<uint8_t>(Response_code::NO_ERROR);
      }
    return packetlen;
  }

}
