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

//#define DEBUG
#include <common>
#include <net/dns/dns.hpp>
#include <net/util.hpp>

#include <string>

using namespace std;

namespace net
{
  std::string parse_dns_query(unsigned char* c)
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

  int DNS::createResponse(DNS::header& hdr, DNS::lookup_func lookup)
  {
    debug("Request ID: %i \n", htons(hdr.id));
    debug("\t Type: %s \n", (hdr.qr ? "RESPONSE" : "QUERY"));

#ifdef DEBUG
    unsigned short qno = ntohs(hdr.q_count);
    debug("Questions: %i \n ", qno);
#endif

    char* buffer = (char*) &hdr + sizeof(header);

    /// NOTE: ASSUMING 1 QUESTION ///
    char* query = buffer;

    std::string parsed_query = parse_dns_query((unsigned char*) query);
    debug("Question: %s\n", parsed_query.c_str());

    buffer += parsed_query.size() + 1; // zero-terminated

#ifdef DEBUG
    question& q = *(question*) buffer;
    unsigned short qtype  = ntohs(q.qtype);
    unsigned short qclass = ntohs(q.qclass);

    debug("Type:  %s (%i)",DNS::question_string(qtype).c_str(), qtype);
    debug("\t Class: %s (%i)",((qclass == 1) ? "INET" : "Unknown class"),qclass);
#endif

    // go to next question (assuming 1 question!!!!)
    buffer += sizeof(question);

    //////////////////////////
    /// RESPONSE PART HERE ///
    //////////////////////////

    // initial response size
    unsigned short packetlen = sizeof(header) +
      sizeof(question) + parsed_query.size() + 1;

    // set DNS QR to RESPONSE
    hdr.qr = DNS_QR_RESPONSE;
    hdr.aa = 1; // authoritah

    // auth & additional = 0, for now
    hdr.auth_count = 0;
    hdr.add_count  = 0;

    std::vector<IP4::addr>* addrs = lookup(parsed_query);
    if (addrs == nullptr)
      {
        // not found
        debug("*** Could not find: %s", parsed_query.c_str());
        hdr.ans_count = 0;
        hdr.rcode     = DNS::NO_ERROR;
      }
    else
      {
        debug("*** Found %lu results for %s", addrs->size(), parsed_query.c_str());
        // append answers
        for (auto addr : *addrs)
          {
            debug("*** Result: %s", addr.str().c_str());
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
            data->data_len = htons(sizeof(IP4::addr));
            buffer += sizeof(rr_data);

            // add resource itself
            *((IP4::addr*) buffer) = addr; // IPv4 address
            buffer += sizeof(IP4::addr);

            packetlen += sizeof(rr_data) + sizeof(IP4::addr); // (!)
          } // addr

        // set dns header answer count (!)
        hdr.ans_count = htons((addrs->size() & 0xFFFF));
        hdr.rcode     = DNS::NO_ERROR;
      }
    return packetlen;
  }


  int DNS::Request::create(char* buffer, const std::string& hostname)
  {
    this->hostname_ = hostname;
    this->answers.clear();
    this->auth.clear();
    this->addit.clear();
    this->id = generateID();

    // fill with DNS request data
    DNS::header* dns = (DNS::header*) buffer;
    dns->id = htons(this->id);
    dns->qr = DNS_QR_QUERY;
    dns->opcode = 0;       // standard query
    dns->aa = 0;           // not Authoritative
    dns->tc = DNS_TC_NONE; // not truncated
    dns->rd = 1; // recursion Desired
    dns->ra = 0; // recursion not available
    dns->z  = DNS_Z_RESERVED;
    dns->ad = 0;
    dns->cd = 0;
    dns->rcode = DNS::resp_code::NO_ERROR;
    dns->q_count = htons(1); // only 1 question
    dns->ans_count  = 0;
    dns->auth_count = 0;
    dns->add_count  = 0;

    // point to the query portion
    char* qname = buffer + sizeof(DNS::header);

    // convert host to dns name format
    dnsNameFormat(qname);
    // length of dns name
    int namelen = strlen(qname) + 1;

    // set question to Internet A record
    DNS::question* qinfo;
    qinfo   = (DNS::question*) (qname + namelen);
    qinfo->qtype  = htons(DNS_TYPE_A); // ipv4 address
    qinfo->qclass = htons(DNS_CLASS_INET);

    // return the size of the message to be sent
    return sizeof(header) + namelen + sizeof(question);
  }

  // parse received message (as put into buffer)
  bool DNS::Request::parseResponse(const char* buffer, size_t len)
  {
    Expects(len >= sizeof(DNS::header));

    const header* dns = (const header*) buffer;

    // move ahead of the dns header and the query field
    const char* reader = buffer + sizeof(DNS::header);
    while (*reader) reader++;
    // .. and past the original question
    reader += sizeof(DNS::question);

    if(UNLIKELY(reader > (buffer + len)))
      return false;

    // parse answers
    for(int i = 0; i < ntohs(dns->ans_count); i++)
      answers.emplace_back(reader, buffer, len);

    // parse authorities
    for (int i = 0; i < ntohs(dns->auth_count); i++)
      auth.emplace_back(reader, buffer, len);

    // parse additional
    for (int i = 0; i < ntohs(dns->add_count); i++)
      addit.emplace_back(reader, buffer, len);

    return true;
  }

  void DNS::Request::print(const char* buffer) const
  {
    header* dns = (header*) buffer;

    printf(" %d questions\n", ntohs(dns->q_count));
    printf(" %d answers\n",   ntohs(dns->ans_count));
    printf(" %d authoritative servers\n", ntohs(dns->auth_count));
    printf(" %d additional records\n\n",  ntohs(dns->add_count));

    // print answers
    for (auto& answer : answers)
      answer.print();

    // print authorities
    for (auto& a : auth)
      a.print();

    // print additional resource records
    for (auto& a : addit)
      a.print();

    printf("\n");
  }

  // convert www.google.com to 3www6google3com
  void DNS::Request::dnsNameFormat(char* dns)
  {
    int lock = 0;

    std::string copy = this->hostname_ + ".";
    int len = copy.size();

    for(int i = 0; i < len; i++)
      {
        if (copy[i] == '.')
          {
            *dns++ = i - lock;
            for(; lock < i; lock++)
              {
                *dns++ = copy[lock];
              }
            lock++;
          }
      }
    *dns++ = '\0';
  }

  DNS::Request::rr_t::rr_t(const char*& reader, const char* buffer, size_t len)
  {
    int stop;

    this->name = readName(reader, buffer, len, stop);
    reader += stop;

    this->resource = *(rr_data*) reader;
    reader += sizeof(rr_data);

    // if its an ipv4 address
    if (ntohs(resource.type) == DNS_TYPE_A)
      {
        int dlen = ntohs(resource.data_len);

        this->rdata = std::string(reader, dlen);
        reader += len;
      }
    else if (ntohs(resource.type) == DNS_TYPE_AAAA)
    {
      // skip IPv6 records for now
      int dlen = ntohs(resource.data_len);
      reader += dlen;
    }
    else
      {
        this->rdata = readName(reader, buffer, len, stop);
        reader += stop;
      }
  }

  IP4::addr DNS::Request::rr_t::getIP4() const
  {
    switch (ntohs(resource.type)) {
    case DNS_TYPE_A:
      return *(IP4::addr*) rdata.data();
    case DNS_TYPE_ALIAS:
    case DNS_TYPE_NS:
    default:
      return IP4::ADDR_ANY;
    }
  }

  void DNS::Request::rr_t::print() const
  {
    printf("Name: %s ", name.c_str());
    switch (ntohs(resource.type))
      {
      case DNS_TYPE_A:
        {
          auto* addr = (IP4::addr*) rdata.data();
          printf("has IPv4 address: %s", addr->str().c_str());
        }
        break;
      case DNS_TYPE_ALIAS:
        printf("has alias: %s", rdata.c_str());
        break;
      case DNS_TYPE_NS:
        printf("has authoritative nameserver : %s", rdata.c_str());
        break;
      default:
        printf("has unknown resource type: %d", ntohs(resource.type));
      }
    printf("\n");
  }

  std::string DNS::Request::rr_t::readName(const char* reader, const char* buffer, size_t tot_len, int& count)
  {
    std::string name(256, '\0');
    unsigned p = 0;
    unsigned offset = 0;
    bool jumped = false;

    count = 1;
    unsigned char* ureader = (unsigned char*) reader;

    while (*ureader)
      {
        if (*ureader >= 192)
          {
            // read 16-bit offset, mask out the 2 top bits
            offset = ((*ureader) * 256 + *(ureader+1)) & 0x3FFF; // = 11000000 00000000

            if(UNLIKELY(offset > tot_len))
              return {};

            ureader = (unsigned char*) buffer + offset - 1;
            jumped = true; // we have jumped to another location so counting wont go up!
          }
        else
          {
            name[p++] = *ureader;
          }
        ureader++;

        // if we havent jumped to another location then we can count up
        if (jumped == false) count++;
      }

    // maximum label size
    if(UNLIKELY(p > 63))
      return {};

    name.resize(p);

    // number of steps we actually moved forward in the packet
    if (jumped)
      count++;

    // now convert 3www6google3com0 to www.google.com
    int len = p; // same as name.size()
    int i;
    for(i = 0; i < len; i++)
      {
        p = name[i];
        for(unsigned j = 0; j < p; j++)
          {
            name[i] = name[i+1];
            i++;
          }
        name[i] = '.';
      }
    name[i - 1] = '\0'; // remove the last dot
    return name;

  } // readName()

}
