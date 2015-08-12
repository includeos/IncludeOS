#include <os>
#include <iostream>
#include <net/inet>
#include <net/class_ip6.hpp>

using namespace std;

#define SERVICE_PORT 5555
net::Inet* network;
/*
int listener(std::shared_ptr<net::Packet>& pckt)
{
  cout << "<DNS SERVER> got packet..." << endl;
  using namespace net;
  
  UDP::full_header& udp = *(UDP::full_header*) pckt->buffer();
  //DNS::header& hdr      = *(DNS::header*) (pckt->buffer() + sizeof(UDP::full_header));
  
  /// create response ///
  int packetlen = 0;
  
  /// send response back to client ///
  
  // set source & return address
  udp.udp_hdr.dport = udp.udp_hdr.sport;
  udp.udp_hdr.sport = htons(SERVICE_PORT);
  udp.udp_hdr.length = htons(sizeof(UDP::udp_header) + packetlen);
  
  // Populate outgoing IP header
  udp.ip_hdr.daddr = udp.ip_hdr.saddr;
  udp.ip_hdr.saddr = network->ip4(ETH0);
  udp.ip_hdr.protocol = IP4::IP4_UDP;
  
  // packet length (??)
  int res = pckt->set_len(sizeof(UDP::full_header) + packetlen); 
  if(!res)
    cout << "<DNS_SERVER> ERROR setting packet length failed" << endl;
  std::cout << "Returning " << packetlen << "b to " << udp.ip_hdr.daddr.str() << std::endl;  
  std::cout << "Full packet size: " << pckt->len() << endl;
  // return packet (as DNS response)
  network->udp_send(pckt);
  
  return 0;
}*/

struct TestAddr
{
  // constructors
  TestAddr()
    : i64{0, 0} {}
  TestAddr(uint32_t a, uint32_t b, uint32_t c, uint32_t d)
    : i32{a, b, c, d} {}
  TestAddr(uint64_t top, uint64_t bot)
    : i64{top, bot} {}
  //TestAddr(__m128i address)
  //  : i128(address) {}
  // copy-constructor
  TestAddr(const TestAddr& a)
  {
    printf("TestAddr copy constructor\n");
    printf("TestAddr %s\n", a.to_string().c_str());
    //i128 = a.i128;
    this->i64[0] = a.i64[0];
    this->i64[1] = a.i64[1];
  }
  // move constructor
  TestAddr& operator= (const TestAddr& a)
  {
    printf("TestAddr move constructor\n");
    printf("TestAddr %s\n", a.to_string().c_str());
    //i128 = a.i128;
    this->i64[0] = a.i64[0];
    this->i64[1] = a.i64[1];
    return *this;
  }
  // returns this IPv6 address as a string
  std::string to_string() const;
  
  //__m128i  i128;
  uint64_t i64[2];
  uint32_t i32[4];
  uint16_t i16[8];
  uint8_t  i8[16];
}; // __attribute__((aligned(16)));

const std::string lut = "0123456789abcdef";

std::string TestAddr::to_string() const
{
  std::string ret(40, 0);
  int counter = 0;
  
  const uint8_t* octet = i8;
  
  for (int i = 0; i < 16; i++)
  {
    ret[counter++] = lut[(octet[i] & 0xF0) >> 4];
    ret[counter++] = lut[(octet[i] & 0x0F) >> 0];
    if (i & 1)
      ret[counter++] = ':';
  }
  ret.resize(counter-1);
  return ret;
}


void Service::start()
{
	std::cout << "*** Service is up - with OS Included! ***" << std::endl;
	
  //using namespace net;
  //auto& mac = Dev::eth(ETH0).mac();
  
  //Inet::ifconfig(ETH0, // Interface
  //    {mac.part[2],mac.part[3],mac.part[4],mac.part[5]}, // IP
  //    {255,255,0,0}, // Netmask
  //    IP6::addr(255, 255) ); // IPv6
  
  TestAddr ip6(1234, 1234);
  
  register void* sp asm ("sp");
  printf("stack: %p\n", sp);
  
  //TestAddr test = ip6;
  std::cout << "ip6 = " << ip6.to_string() << std::endl;
  //printf("ipv6 %s\n", ip6.to_string().c_str());
  
	//network = Inet::up();
  //std::cout << "Starting UDP server on IP " 
  //          << network->ip4(ETH0).str() << std::endl;
}
