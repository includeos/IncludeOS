
#include <os>
#include <net/interfaces>

using namespace net;

void Service::start(const std::string&)
{
  auto& inet = Interfaces::get(0);
  inet.network_config({  10,  0,  0, 45 },    // IP
                      { 255, 255, 0,  0 },    // Netmask
                      {  10,  0,  0,  1 },    // Gateway
                      {   8,  8,  8,  8 }     // DNS
  );
  printf("Service IP address is %s\n", inet.ip_addr().str().c_str());

  // ping gateway
  inet.icmp().ping(inet.gateway(), [](ICMP_view pckt) {
    if (pckt)
      printf("Received packet from gateway\n%s\n", pckt.to_string().c_str());
    else
      printf("No reply received from gateway\n");
  });

  /* If IP forwarding on:
  inet.icmp().ping("google.com", [](ICMP_view pckt) {
    if (pckt)
      printf("Received packet\n%s\n", pckt.to_string().c_str());
    else
      printf("No reply received from google.com. Identifier: %u. Sequence number: %u\n", pckt.id(), pckt.seq());
  });

  // ping google.com with callback
  inet.icmp().ping(IP4::addr{193,90,147,109}, [](ICMP_view pckt) {
    if (pckt)
      printf("Received packet\n%s\n", pckt.to_string().c_str());
    else
      printf("No reply received from 193.90.147.109. Identifier: %u. Sequence number: %u\n", pckt.id(), pckt.seq());
  });
  */

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

  // Also possible without callback:
  // inet.icmp().ping("google.com");
  // inet.icmp().ping(IP4::addr{193,90,147,109});  // google.com
}
