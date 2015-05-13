#include <os>
#include <net/inet>
#include <net/util.hpp>
#include <net/class_packet.hpp>
#include <net/class_udp.hpp>
#include <string>
#include <iostream>

#include <memstream>
#include "packet_store.hpp"

using namespace net;
Inet* network;
PacketStore packetStore(50, 1500);

void reflect(Inet*, std::shared_ptr<Packet>&);

static const int SERVICE_PORT = 12345;

void Service::start()
{
  Inet::ifconfig(net::ETH0,
                 { 10,  0, 0, 2},
                 {255,255, 0, 0},
                 {0, 0});
	
	network = Inet::up();
	
  network->udp_listen(SERVICE_PORT,
  [] (std::shared_ptr<net::Packet>& pckt)
  {
    std::cout << "*** Received data on port " << SERVICE_PORT << std::endl;
    reflect(network, pckt);
    
    return 0;
  });
}

void reflect(Inet* network, std::shared_ptr<Packet>& packet)
{
  //auto pckt = packetStore.getPacket();
  UDP::full_header& header = *(UDP::full_header*) packet->buffer();
  
  // Populate outgoing UDP header
  header.udp_hdr.dport = header.udp_hdr.sport;
  header.udp_hdr.sport = htons(SERVICE_PORT);
  //header.udp_hdr.length = htons(sizeof(UDP::udp_header) + messageSize);
  
  // Populate outgoing IP header
  header.ip_hdr.daddr = header.ip_hdr.saddr;
  header.ip_hdr.saddr = network->ip4(ETH0);
  header.ip_hdr.protocol = IP4::IP4_UDP;
  
  // packet payload
  //streamucpy((char*) pckt->buffer() + sizeof(UDP::full_header), this->buffer, messageSize);
  //pckt->set_len(sizeof(UDP::full_header) + messageSize);
  
  std::cout << "*** Responding to " << header.ip_hdr.daddr.str() << ", "
            << "len = " << htons(header.udp_hdr.length) << std::endl;
  network->udp_send(packet);
}
