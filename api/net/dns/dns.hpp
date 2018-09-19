// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015-2018 Oslo and Akershus University College of Applied Sciences
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

#include <net/ip4/addr.hpp> // ip4::Addr
#include <delegate>
#include <string>
#include <vector>

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

namespace net::dns {

  using id_t = uint16_t;

  static constexpr uint16_t SERVICE_PORT = 53;

  class Response;
  using Response_ptr = std::unique_ptr<Response>;

  enum class Record_type : uint16_t
  {
    A     = 1,
    NS    = 2,
    ALIAS = 5,
    AAAA  = 28
  };

  enum class Class : uint16_t
  {
    INET = 1
  };

  struct Header
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

  struct Question
  {
    unsigned short qtype;
    unsigned short qclass;
  } __attribute__ ((packed));

  enum class Response_code : uint8_t
  {
    NO_ERROR     = 0,
    FORMAT_ERROR = 1,
    SERVER_FAIL  = 2,
    NAME_ERROR   = 3,
    NOT_IMPL     = 4, // unimplemented feature
    OP_REFUSED   = 5, // for political reasons
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

  // convert www.google.com to 3www6google3com
  int encode_name(std::string name, char* dst);
  // convert 3www6google3com to www.google.com
  //int decode_name(...);

  typedef delegate<std::vector<ip4::Addr>* (const std::string&)> lookup_func;
  int create_response(Header& hdr, lookup_func func);

}

#endif
