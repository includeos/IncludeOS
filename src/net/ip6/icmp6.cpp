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

//#define DEBUG
#include <net/ip6/icmp6.hpp>
#include <net/ip6/ndp.hpp>

#include <iostream>
#include <net/ip6/ip6.hpp>
#include <alloca.h>
#include <assert.h>

namespace net
{
  // internal implementation of handler for ICMP type 128 (echo requests)
  int echo_request(ICMPv6&, std::shared_ptr<PacketICMP6>& pckt);
  int neighbor_solicitation(ICMPv6& caller, std::shared_ptr<PacketICMP6>& pckt);
  
  ICMPv6::ICMPv6(IP6::addr& local_ip)
    : localIP(local_ip)
  {
    // install default handler for echo requests
    listen(ECHO_REQUEST, echo_request);
    // install default handler for neighbor solicitation requests
    listen(ND_NEIGHB_SOL, neighbor_solicitation);
  }
  
  // static function that returns a textual representation of
  // the @code and @type of an ICMP message
  std::string ICMPv6::code_string(uint8_t type, uint8_t code)
  {
    switch (type)
      {
        /// error codes ///
      case 1:
        /// delivery problems ///
        switch (code)
          {
          case 0:
            return "No route to destination";
          case 1:
            return "Communication with dest administratively prohibited";
          case 2:
            return "Beyond scope of source address";
          case 3:
            return "Address unreachable";
          case 4:
            return "Port unreachable";
          case 5:
            return "Source address failed ingress/egress policy";
          case 6:
            return "Reject route to destination";
          case 7:
            return "Error in source routing header";
          default:
            return "ERROR Invalid ICMP type";
          }
      case 2:
        /// size problems ///
        return "Packet too big";
      
      case 3:
        /// time problems ///
        switch (code)
          {
          case 0:
            return "Hop limit exceeded in traffic";
          case 1:
            return "Fragment reassembly time exceeded";
          default:
            return "ERROR Invalid ICMP code";
          }
      case 4:
        /// parameter problems ///
        switch (code)
          {
          case 0:
            return "Erroneous header field";
          case 1:
            return "Unrecognized next header";
          case 2:
            return "Unrecognized IPv6 option";
          default:
            return "ERROR Invalid ICMP code";
          }
      
        /// echo feature ///
      case ECHO_REQUEST:
        return "Echo request";
      case ECHO_REPLY:
        return "Echo reply";
      
        /// multicast feature ///
      case 130:
        return "Multicast listener query";
      case 131:
        return "Multicast listener report";
      case 132:
        return "Multicast listener done";
      
        /// neighbor discovery protocol ///
      case ND_ROUTER_SOL:
        return "NDP Router solicitation request";
      case ND_ROUTER_ADV:
        return "NDP Router advertisement";
      case ND_NEIGHB_SOL:
        return "NDP Neighbor solicitation request";
      case ND_NEIGHB_ADV:
        return "NDP Neighbor advertisement";
      case ND_REDIRECT:
        return "NDP Redirect message";
      
      case 143:
        return "Multicast Listener Discovery (MLDv2) reports (RFC 3810)";
      
      default:
        return "Unknown type: " + std::to_string((int) type);
      }
  }
  
  int ICMPv6::bottom(Packet_ptr pckt)
  {
    auto icmp = std::static_pointer_cast<PacketICMP6>(pckt);
    
    type_t type = icmp->type();
    
    if (listeners.find(type) != listeners.end())
      {
        return listeners[type](*this, icmp);
      }
    else
      {
        debug(">>> IPv6 -> ICMPv6 bottom (no handler installed)\n");
        debug("ICMPv6 type %d: %s\n", 
              (int) icmp->type(), code_string(icmp->type(), icmp->code()).c_str());
      
        /*
        // show correct checksum
        intptr_t chksum = icmp->checksum();
        debug("ICMPv6 checksum: %p \n",(void*) chksum);
      
        // show our recalculated checksum
        icmp->header().checksum_ = 0;
        chksum = checksum(icmp);
        debug("ICMPv6 our estimate: %p \n", (void*) chksum );
        */
        return -1;
      }
  }
  int ICMPv6::transmit(std::shared_ptr<PacketICMP6>& pckt)
  {
    // NOTE: *** OBJECT CREATED ON STACK *** -->
    auto original = std::static_pointer_cast<PacketIP6>(pckt);
    // NOTE: *** OBJECT CREATED ON STACK *** <--
    return ip6_out(original);
  }
  
  uint16_t ICMPv6::checksum(std::shared_ptr<PacketICMP6>& pckt)
  {
    IP6::header& hdr = pckt->ip6_header();
    
    uint16_t datalen = hdr.size();
    pseudo_header phdr;
    
    // ICMP checksum is done with a pseudo header
    // consisting of src addr, dst addr, message length (32bits)
    // 3 zeroes (8bits each) and id of the next header
    phdr.src = hdr.src;
    phdr.dst = hdr.dst;
    phdr.len = htonl(datalen);
    phdr.zeros[0] = 0;
    phdr.zeros[1] = 0;
    phdr.zeros[2] = 0;
    phdr.next = hdr.next();
    //assert(hdr.next() == 58); // ICMPv6
    
    /**
       RFC 4443
       2.3. Message Checksum Calculation
      
       The checksum is the 16-bit one's complement of the one's complement
       sum of the entire ICMPv6 message, starting with the ICMPv6 message
       type field, and prepended with a "pseudo-header" of IPv6 header
       fields, as specified in [IPv6, Section 8.1].  The Next Header value
       used in the pseudo-header is 58.  (The inclusion of a pseudo-header
       in the ICMPv6 checksum is a change from IPv4; see [IPv6] for the
       rationale for this change.)
      
       For computing the checksum, the checksum field is first set to zero.
    **/
    union
    {
      uint32_t whole;
      uint16_t part[2];
    } sum;
    sum.whole = 0;
    
    // compute sum of pseudo header
    uint16_t* it = (uint16_t*) &phdr;
    uint16_t* it_end = it + sizeof(pseudo_header) / 2;
    
    while (it < it_end)
      sum.whole += *(it++);
    
    // compute sum of data
    it = (uint16_t*) pckt->payload();
    it_end = it + datalen / 2;
    
    while (it < it_end)
      sum.whole += *(it++);
    
    // odd-numbered case
    if (datalen & 1)
      sum.whole += *(uint8_t*) it;
    
    return ~(sum.part[0] + sum.part[1]);
  }
  
  // internal implementation of handler for ICMP type 128 (echo requests)
  int echo_request(ICMPv6& caller, std::shared_ptr<PacketICMP6>& pckt)
  {
    ICMPv6::echo_header* icmp = (ICMPv6::echo_header*) pckt->payload();
    debug("*** Custom handler for ICMP ECHO REQ type=%d 0x%x\n", icmp->type, htons(icmp->checksum));
    
    // set the hoplimit manually to the very standard 64 hops
    pckt->set_hoplimit(64);
    
    // set to ICMP Echo Reply (129)
    icmp->type = ICMPv6::ECHO_REPLY;
    
    if (pckt->dst().is_multicast())
      {
        // We won't be changing source address for multicast ping
        debug("Was multicast ping6: no change for source and dest\n");
      }
    else
      {
        printf("Normal ping6: source is us\n");
        printf("src is %s\n", pckt->src().str().c_str());
        printf("dst is %s\n", pckt->dst().str().c_str());
      
        printf("multicast is %s\n", IP6::addr::link_all_nodes.str().c_str());
        // normal ping: send packet to source, from us
        pckt->set_dst(pckt->src());
        pckt->set_src(caller.local_ip());
      }
    // calculate and set checksum
    // NOTE: do this after changing packet contents!
    icmp->checksum = 0;
    icmp->checksum = ICMPv6::checksum(pckt);
    
    // send packet downstream
    return caller.transmit(pckt);
  }
  int neighbor_solicitation(ICMPv6& caller, std::shared_ptr<PacketICMP6>& pckt)
  {
    (void) caller;
    NDP::neighbor_sol* sol = (NDP::neighbor_sol*) pckt->payload();
    
    printf("ICMPv6 NDP Neighbor solicitation request\n");
    printf(">> target: %s\n", sol->target.str().c_str());
    printf(">>\n");
    printf(">> source: %s\n", pckt->src().str().c_str());
    printf(">> dest:   %s\n", pckt->dst().str().c_str());
    
    // perhaps we should answer
    (void) caller;
    
    return -1;
  }
  
  void ICMPv6::discover()
  {
    // ether-broadcast an IPv6 packet to all routers
    // IPv6mcast_02: 33:33:00:00:00:02
    auto pckt = IP6::create(
                            IP6::PROTO_ICMPv6,
                            Ethernet::addr::IPv6mcast_02, 
                            IP6::addr::link_unspecified);
    
    // RFC4861 4.1. Router Solicitation Message Format
    pckt->set_hoplimit(255);
    
    NDP::router_sol* ndp = (NDP::router_sol*) pckt->payload();
    // set to Router Solicitation Request
    ndp->type = ICMPv6::ND_ROUTER_SOL;
    ndp->code = 0;
    ndp->checksum = 0;
    ndp->reserved = 0;
    
    auto icmp = std::static_pointer_cast<PacketICMP6> (pckt);
    
    // source and destination addresses
    icmp->set_src(this->local_ip()); //IP6::addr::link_unspecified);
    icmp->set_dst(IP6::addr::link_all_routers);
    
    // ICMP header length field
    icmp->set_length(sizeof(NDP::router_sol));
    
    // calculate and set checksum
    // NOTE: do this after changing packet contents!
    ndp->checksum = ICMPv6::checksum(icmp);
    
    this->transmit(icmp);
    
    /// DHCPv6 test ///
    // ether-broadcast an IPv6 packet to all routers
    //pckt = IP6::create(
    //    IP6::PROTO_UDP,
    //    Ethernet::addr::IPv6mcast_02, 
    //    IP6::addr::link_unspecified);
  }
  
  
}
