#include <os>
#include <stdio.h>
#include <net/inet>

void Service::start()
{
  auto& mac = Dev::eth(0).mac();
  net::IP6::addr ip6(1, 2, 3, 4, 5, 6, 7, 8);
  
  // verify equality operators
  net::IP6::addr test1(9, 2, 3, 4, 5, 6, 7, 8);
  net::IP6::addr test2(9, 2, 3, 4, 5, 6, 7, 8);
  
  assert(ip6 == ip6);
  assert(ip6 != test1);
  assert(ip6 != test2);
  assert(test1 == test2);
  
  // verify that multicast addresses are multicast
  assert(net::IP6::addr::mcast1.is_multicast());
  assert(net::IP6::addr::mcast2.is_multicast());
  
  // basic UDP service
  net::Inet::ifconfig(
      net::ETH0, 
      {{ mac.part[2],mac.part[3],mac.part[4],mac.part[5] }}, 
      {{255, 255, 255, 0}}, 
      ip6);
  
  net::Inet* inet = net::Inet::up();
  
  printf("Service IP4 address: %s\n", net::Inet::ip4(net::ETH0).str().c_str());
  printf("Service IP6 address: %s\n", net::Inet::ip6(net::ETH0).str().c_str());
  
  /// Using multicast we can see the packet from Linux:
  /// > nc -6u ff02::2%include0 64
  
  /// Multicast ping:
  /// > ping6 ff02::1 -I include0
  
  /// Regular ping:
  /// > 
  
  // basic UDP service
  static const int UDP_PORT = 64;
  inet->udp6_listen(UDP_PORT,
    [=] (std::shared_ptr<net::PacketUDP6>& pckt) -> int
    {
      printf("Received UDP6 packet from %s to my listener on port %d\n",
          pckt->src().str().c_str(), pckt->dst_port());
      
      std::string data((const char*) pckt->data(), pckt->data_length());
      
      printf("Contents (len=%d):\n%s\n", pckt->data_length(), data.c_str());
      
      // unfortunately,
      // copy the ether src field of the incoming packet
      net::Ethernet::addr ether_src = 
          ((net::Ethernet::header*) pckt->buffer())->src;
      
      // create a response packet with destination [ether_src] dst()
      std::shared_ptr<net::PacketUDP6> newpacket = 
          inet->udp6_create(ether_src, pckt->dst(), UDP_PORT);
      
      const char* text = "This is the response packet!";
      // copy text into UDP data section
      memcpy( newpacket->data(),  text,  strlen(text) );
      // set new length
      newpacket->set_length(strlen(text));
      
      // generate checksum for packet before sending
      newpacket->gen_checksum();
      
      // ship it to the ether
      inet->udp6_send(newpacket);
      return -1;
    }
  );
  
}
