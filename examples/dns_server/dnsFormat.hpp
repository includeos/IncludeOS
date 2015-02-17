#ifndef DNSFORMAT_HPP
#define DNSFORMAT_HPP

#include <stdint.h>

union macaddr{
  uint8_t part[6];
  struct {
    uint16_t minor;
    uint32_t major;
  } __attribute__((packed));
}__attribute__((packed));

// 14 bytes (...!)
struct header {
  macaddr dest;
  macaddr src;
  unsigned short type;
}__attribute__((packed));

union __attribute__((packed)) v4addr{
  uint8_t part[4];
  uint32_t whole;
};

// 20 bytes
/** IP4 header */
struct ip_header {
  uint8_t version_ihl;
  uint8_t tos;
  uint16_t tot_len;
  uint16_t id;
  uint16_t frag_off_flags;
  uint8_t ttl;
  uint8_t protocol;
  uint16_t check;
  v4addr saddr;
  v4addr daddr;
}__attribute__((packed));

/** UDP port number */
typedef uint16_t port;

/** A protocol buffer temporary type. Later we might encapsulate.*/
//typedef uint8_t* pbuf;

// 8 bytes
/** UDP header */
struct udp_header {
  port sport;
  port dport;
  uint16_t length;
  uint16_t checksum;
}__attribute__((packed));

// 40 bytes
/** Full UDP Header with all sub-headers */
struct full_header{
  header eth_hdr;
  ip_header ip_hdr;
  udp_header udp_hdr;
}__attribute__((packed));

// Note shifted byte order (endianness)
#define DNS_QUERY    0x0001 // Can alternatively use ntohs(0x0000)
#define DNS_RESPONSE 0x8081 // Can alternatively use ntohs(0x8000)

//Defining the 12 byte header
struct __attribute__ ((packed)) DnsHeaderQuery {
  // identifiction can be random, just an example here:
  uint16_t identification = 0x241a; // remember endianness
  uint16_t flags = DNS_QUERY;
  uint8_t noq_zero  = 0x00;
  uint8_t no_questions  = 0x01;
  uint8_t noa_zero  = 0x00;
  uint8_t no_answers    = 0x00;
  uint16_t no_authority  = 0x0000; 
  uint16_t no_additional = 0x0000;
};

//Defining the 12 byte header (in total, 52 bytes so far...)
struct __attribute__ ((packed)) DnsHeaderResponse {
  // identifiction can be random, just an example here:
  uint16_t identification = 0x241a; // remember endianness
  uint16_t flags = DNS_RESPONSE;
  uint8_t noq_zero  = 0x00;
  uint8_t no_questions  = 0x01;
  uint8_t noa_zero  = 0x00;
  uint8_t no_answers    = 0x01;
  uint16_t no_authority  = 0x0000; 
  uint16_t no_additional = 0x0000;
};

// Defining 25 bytes (in total, 77 bytes so far...)
struct __attribute__ ((packed)) ServerFQDN {
  // Adding "www" question part
  //uint8_t server_label_length = 0x03; // MUST be < 64 (i.e. < 0x40)
  //uint8_t w1 = 0x77;
  //uint8_t w2 = 0x77;
  //uint8_t w3 = 0x77;

  // Adding "serv-00000" question part
  // Note that the server number goes into byte 19 (char[18]) to 23 (char[22]) form dataStart
  uint8_t server_label_length = 0x0a; // MUST be < 64 (i.e. < 0x40)
  uint32_t serv =0x76726573;      // remember endianness
  //uint8_t serv1 = 0x73;
  //uint8_t serv2 = 0x65;
  //uint8_t serv3 = 0x72;
  //uint8_t serv4 = 0x76;
  uint32_t numpart1 = 0x3030302D; // remember endianness
  uint16_t numpart2 = 0x3030;

  // Adding "mydomain" question part
  uint8_t domain_label_length = 0x08; // MUST be < 64 (i.e. < 0x40)

  uint32_t mydo = 0x6F64796D; // remember endianness
  uint32_t main = 0x6E69616D; // remember endianness
  // ADDING "com." question part
  uint8_t tld_label_length = 0x03; // MUST be < 64 (i.e. < 0x40)
  uint32_t com = 0x006D6F63; // remember endianness
};

// Defining another 4 bytes (in total, 81 bytes so far...)
struct __attribute__ ((packed)) DnsQuestion {
  ServerFQDN namePart;
  uint16_t questionType = 0x0100;   // ntohs(0x0001) means A-record (IPv4-address) is requested
  uint16_t questionClass = 0x0100;   // ntohs(0x0001) means IN (Internet)
};

// Defining another 14 bytes (in total 95 bytes so far...)
struct __attribute__ ((packed)) DnsArecordResponse {
  uint16_t questionType = 0x0100;   // ntohs(0x0001) means A-record (IPv4-address) is requested
  uint16_t questionClass = 0x0100;   // ntohs(0x0001) means IN (Internet)
  uint32_t ttl = 0x00010000;
  uint8_t rdL_zero = 0x00; // remember endianness
  uint8_t rdLength = 0x00; // remember endianness
  uint32_t ipv4Address = 0x0100000a; // remember endianness
};

struct __attribute__ ((packed)) DnsQuery {
  DnsHeaderQuery dnsHeaderQuery;
  DnsQuestion dnsQuestion;
};

struct __attribute__ ((packed)) DnsResponse {
  DnsHeaderResponse dnsHeaderQuery;
  DnsQuestion dnsQuestion;
  DnsArecordResponse dnsArecordResponse;
};

struct __attribute__ ((packed)) DnsQueryPacket {
  full_header fullHeader; 
  DnsQuery dnsQuery;
};

struct __attribute__ ((packed)) DnsResponsePacket {
  full_header fullHeader; 
  DnsResponse dnsResponse;
};

//DnsHeader(uint16_t message_type);

//void
//DnsHeader::DnsHeader(uint16_t message_type) {
//  flags = message_type;
//  if (message_type == DNS_RESPONSE)
//    no_answers = 0x0001;
//}


#endif
