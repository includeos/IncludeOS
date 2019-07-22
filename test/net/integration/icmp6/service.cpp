
#include <os>
#include <net/interfaces>

using namespace net;

void Service::start()
{
#ifdef USERSPACE_LINUX
  extern void create_network_device(int N, const char* route, const char* ip);
  create_network_device(0, "10.0.0.0/24", "10.0.0.1");
#endif

  auto& inet = Interfaces::get(0);
  inet.network_config({  10,  0,  0, 52 },    // IP
                      { 255, 255, 0,  0 },    // Netmask
                      {  10,  0,  0,  1 },    // Gateway
                      {   8,  8,  8,  8 }     // DNS
  );

  inet.add_addr({"fe80::e823:fcff:fef4:85bd"});
  ip6::Addr gateway{"fe80::e823:fcff:fef4:83e7"};

  printf("Service IPv4 address: %s, IPv6 address: %s\n",
          inet.ip_addr().str().c_str(), inet.ip6_addr().str().c_str());

  const int wait = 10;

  // ping gateway
  inet.icmp6().ping(gateway, [](ICMP6_view pckt) {
    //something is off with the fwd ?
    if (pckt)
      printf("Received packet from gateway\n%s\n", pckt.to_string().c_str());
    else
      printf("No reply received from gateway\n");
  },wait);

#if 0
  const int wait = 10;

  // No reply-pings
  // Waiting 30 seconds for reply
  inet.icmp().ping(IP4::addr{10,0,0,42}, [](ICMP_view pckt) {
    if (pckt)
      printf("Received packet from 10.0.0.42\n%s\n", pckt.to_string().c_str());
    else
      printf("No reply received from 10.0.0.42\n");
  }, wait);

  // Waiting 30 seconds for reply
  inet.icmp().ping(IP4::addr{10,0,0,43}, [](ICMP_view pckt) {
    if (pckt)
      printf("Received packet from 10.0.0.43\n%s\n", pckt.to_string().c_str());
    else
      printf("No reply received from 10.0.0.43\n");
  }, wait);
#endif
}
