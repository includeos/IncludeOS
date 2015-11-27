#pragma once

#include "../arp.hpp"
#include <net/packet.hpp>

namespace net
{
  class PacketArp : public Packet,
                    public std::enable_shared_from_this<PacketArp>
  {
  public:
    
    Arp::header& header() const
    {
      return *(Arp::header*) buffer();
    }
    
    static const size_t headers_size = sizeof(Arp::header);
    
    /** initializes to a default, empty Arp packet, given
	a valid MTU-sized buffer */
    void init(Ethernet::addr local_mac, IP4::addr local_ip)
    {            
      auto& hdr = header();
      hdr.ethhdr.type = Ethernet::ETH_ARP;
      hdr.htype = Arp::H_htype_eth;
      hdr.ptype = Arp::H_ptype_ip4;
      hdr.hlen_plen = Arp::H_hlen_plen;
      
      hdr.dipaddr = next_hop();
      hdr.sipaddr = local_ip;
      hdr.shwaddr = local_mac;
    }
    
    void set_dest_mac(Ethernet::addr mac){
      header().dhwaddr = mac;
      header().ethhdr.dest = mac; 
    } 
    
    void set_opcode(Arp::Opcode op){
      header().opcode = op;
    }
    
    void set_dest_ip(IP4::addr ip){
      header().dipaddr = ip;
    }
    
    IP4::addr source_ip(){
      return header().sipaddr;
    }
    
    IP4::addr dest_ip(){
      return header().dipaddr;
    }
    
    Ethernet::addr source_mac(){
      return header().ethhdr.src;
    };
    
    Ethernet::addr dest_mac(){
      return header().ethhdr.dest;
    };
    
    
  };
}
