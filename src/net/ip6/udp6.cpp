#include <net/ip6/udp6.hpp>

#include <net/util.hpp>
#include <alloca.h>
#include <stdio.h>

namespace net
{
  int UDPv6::bottom(std::shared_ptr<Packet>& pckt)
  {
    printf(">>> IPv6 -> UDPv6 bottom\n");
    auto P6 = std::static_pointer_cast<PacketUDP6>(pckt);
    
    printf(">>> src port: %u \t dst port: %u\n", P6->src_port(), P6->dst_port());
    printf(">>> length: %d   \t chksum: 0x%x\n", P6->length(), P6->checksum());
    
    port_t port = P6->dst_port();
    
    // check for listeners on dst port
    if (listeners.find(port) != listeners.end())
    {
      // make the call to the listener on that port
      return listeners[port](P6);
    }
    // was not forwarded, so just return -1
    printf("... dumping packet, no listeners\n");
    return -1;
  }
  
  int UDPv6::transmit(std::shared_ptr<PacketUDP6>& pckt)
  {
    // NOTE: *** OBJECT CREATED ON STACK *** -->
    auto original = std::static_pointer_cast<Packet>(pckt);
    // NOTE: *** OBJECT CREATED ON STACK *** <--
    return ip6_out(original);
  }
  
  uint16_t PacketUDP6::gen_checksum() const
  {
    IP6::full_header& full = *(IP6::full_header*) this->buffer();
    IP6::header& hdr = full.ip6_hdr;
    
    // ICMP message + pseudo header
    uint16_t datalen = hdr.size() + sizeof(UDPv6::pseudo_header);
    
    // allocate it on stack
    char* data = (char*) alloca(datalen + 16);
    // unfortunately we also need to guarantee SSE aligned
    data = (char*) ((intptr_t) (data+16) & ~15); // P2ROUNDUP((intptr_t) data, 16);
    // verify that its SSE aligned
    assert(((intptr_t) data & 15) == 0);
    
    // ICMP checksum is done with a pseudo header
    // consisting of src addr, dst addr, message length (32bits)
    // 3 zeroes (8bits each) and id of the next header
    UDPv6::pseudo_header& phdr = *(UDPv6::pseudo_header*) data;
    phdr.src = hdr.src;
    phdr.dst = hdr.dst;
    phdr.len = htonl(hdr.size());
    phdr.zeros[0] = 0;
    phdr.zeros[1] = 0;
    phdr.zeros[2] = 0;
    phdr.next = hdr.next();
    
    // normally we would start at &icmp_echo::type, but
    // it is after all the first element of the icmp message
    memcpy(data + sizeof(UDPv6::pseudo_header), this->payload(),
        datalen - sizeof(UDPv6::pseudo_header));
    
    // calculate csum and free data on return
    return net::checksum(data, datalen);
  }
  
  std::shared_ptr<PacketUDP6> UDPv6::create(
      Ethernet::addr ether_dest, const IP6::addr& ip_dest, UDPv6::port_t port)
  {
    // arbitrarily big buffer
    uint8_t* data = new uint8_t[1500];
    Packet* packet = new Packet(data, sizeof(data), Packet::AVAILABLE);
    
    IP6::full_header& full = *(IP6::full_header*) packet->buffer();
    // people dont think that it be, but it do
    full.eth_hdr.type = Ethernet::ETH_IP6;
    full.eth_hdr.dest = ether_dest;
    
    IP6::header& hdr = full.ip6_hdr;
    
    // set IPv6 packet parameters
    hdr.src = this->localIP;
    hdr.dst = ip_dest;
    hdr.init_scan0();
    hdr.set_next(IP6::PROTO_UDP);
    hdr.set_hoplimit(64);
    
    // offset of UDPv6 packet
    packet->set_payload(packet->buffer() + sizeof(IP6::full_header));
    UDPv6::header& udp = *(UDPv6::header*) packet->payload();
    
    // set UDPv6 parameters
    udp.src_port = htons(666); /// FIXME: use free local port
    udp.dst_port = htons(port);
    udp.chksum   = 0;
    
    auto udp_packet = std::static_pointer_cast<PacketUDP6> ( std::shared_ptr<Packet> (packet) );
    
    // make the packet empty
    udp_packet->set_length(0);
    // now, free to use :)
    return udp_packet;
  }
}
