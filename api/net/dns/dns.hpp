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

#ifndef NET_DNS_DNS_HPP
#define NET_DNS_DNS_HPP

/**
 * DNS message
 * +---------------------+
 * | Header              |
 * +---------------------+
 * | Question            | the question for the name server
 * +---------------------+
 * | Answer              | RRs answering the question
 * +---------------------+
 * | Authority           | RRs pointing toward an authority
 * +---------------------+
 * | Additional          | RRs holding additional information
 * +---------------------+
 *
 * DNS header
 * +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 * |                     ID                        |
 * +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 * |QR| Opcode    |AA|TC|RD|RA| Z      |  RCODE    |
 * +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 * |                   QDCOUNT                     |
 * +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 * |                   ANCOUNT                     |
 * +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 * |                   NSCOUNT                     |
 * +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 * |                   ARCOUNT                     |
 * +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 *
 **/

#include <net/ip4/ip4.hpp> // IP4::addr
#include <string>
#include <vector>

namespace net
{
#define DNS_QR_QUERY     0
#define DNS_QR_RESPONSE  1

#define DNS_TC_NONE    0 // no truncation
#define DNS_TC_TRUNC   1 // truncated message

#define DNS_CLASS_INET   1

#define DNS_TYPE_A    1  // A record
#define DNS_TYPE_AAAA 28 // AAAA record
#define DNS_TYPE_NS   2  // respect mah authoritah
#define DNS_TYPE_ALIAS 5 // name alias

#define DNS_TYPE_SOA  6  // start of authority zone
#define DNS_TYPE_PTR 12  // domain name pointer
#define DNS_TYPE_MX  15  // mail routing information

#define DNS_Z_RESERVED   0


  class DNS
  {
  public:
    static const unsigned short DNS_SERVICE_PORT = 53;

    struct header
    {
      unsigned short id;       // identification number
      unsigned char rd :1;     // recursion desired
      unsigned char tc :1;     // truncated message
      unsigned char aa :1;     // authoritive answer
      unsigned char opcode :4; // purpose of message
      unsigned char qr :1;     // query/response flag
      unsigned char rcode :4;  // response code
      unsigned char cd :1;     // checking disabled
      unsigned char ad :1;     // authenticated data
      unsigned char z :1;      // reserved, set to 0
      unsigned char ra :1;     // recursion available
      unsigned short q_count;    // number of question entries
      unsigned short ans_count;  // number of answer entries
      unsigned short auth_count; // number of authority entries
      unsigned short add_count;  // number of resource entries
    } __attribute__ ((packed));

    struct question
    {
      unsigned short qtype;
      unsigned short qclass;
    };

#pragma pack(push, 1)
    struct rr_data // resource record data
    {
      unsigned short type;
      unsigned short _class;
      unsigned int   ttl;
      unsigned short data_len;
    };
#pragma pack(pop)

    enum resp_code
      {
        NO_ERROR     = 0,
        FORMAT_ERROR = 1,
        SERVER_FAIL  = 2,
        NAME_ERROR   = 3,
        NOT_IMPL     = 4, // unimplemented feature
        OP_REFUSED   = 5, // for political reasons
      };

    typedef delegate<std::vector<IP4::addr>* (const std::string&)> lookup_func;

    static int createResponse(header& hdr, lookup_func func);

    static std::string question_string(unsigned short type)
    {
      switch (type)
        {
        case DNS_TYPE_A:
          return "IPv4 address";
        case DNS_TYPE_ALIAS:
          return "Alias";
        case DNS_TYPE_MX:
          return "Mail exchange";
        case DNS_TYPE_NS:
          return "Name server";
        default:
          return "FIXME DNS::question_string(type = " + std::to_string(type) + ")";
        }
    }

    class Request
    {
    public:
      using id_t = unsigned short;
      int  create(char* buffer, const std::string& hostname);
      bool parseResponse(const char* buffer, size_t len);
      void print(const char* buffer) const;

      const std::string& hostname() const
      {
        return this->hostname_;
      }

      IP4::addr getFirstIP4() const
      {
        for(auto&& ans : answers)
        {
          if(ans.is_type(DNS_TYPE_A))
            return ans.getIP4();
        }

        return IP4::ADDR_ANY;
      }

      id_t get_id() const
      { return id; }

    private:
      struct rr_t // resource record
      {
        rr_t(const char*& reader, const char* buffer, size_t len);

        std::string name;
        std::string rdata;
        rr_data resource;

        IP4::addr getIP4() const;
        void      print()  const;

        bool is_type(int type) const
        { return ntohs(resource.type) == type; }

      private:
        // decompress names in 3www6google3com format
        std::string readName(const char* reader, const char* buffer, size_t len, int& count);
      };

      unsigned short generateID()
      {
        static unsigned short id = 0;
        return ++id;
      }
      void dnsNameFormat(char* dns);

      unsigned short id;
      std::string    hostname_;

      std::vector<rr_t> answers;
      std::vector<rr_t> auth;
      std::vector<rr_t> addit;
    };

  };

}

#endif
