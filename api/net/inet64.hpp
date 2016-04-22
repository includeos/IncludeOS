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

#ifndef NET_INET_HPP
#define NET_INET_HPP

#include <syscalls.hpp> // panic()
#include <dev.hpp> // 107: auto& eth0 = Dev::eth(0);
#include <net/ethernet.hpp>
#include <net/arp.hpp>
#include <net/ip4.hpp>
#include <net/udp.hpp>
#include "ip6/ip6.hpp"
#include "ip6/icmp6.hpp"
#include "ip6/udp6.hpp"


#include <vector>

#include <nic.hpp>
#include "ip4/icmpv4.hpp"

namespace net {

  /** Nic names. Only used to bind nic to IP. */
  enum netdev {ETH0,ETH1,ETH2,ETH3,ETH4,ETH5,ETH6,ETH7,ETH8,ETH9};

  /** A (supposed to be) complete IP4 and IP6 dual stack */  
  class Inet64 {
  public:
    /** Listen to a UDP port. 
        This is just a simple forwarder. @see UDP::listen.  */
    inline void udp_listen(uint16_t port, UDP::listener l)
    { _udp.listen(port,l); }
    
    /** Send a UDP datagram. 
        
        @note the data buffer is the *whole* ethernet frame, so don't overwrite 
        headers unless you own them (i.e. you *are* the IP object)  */
    inline int udp_send(std::shared_ptr<Packet> pckt)
    { return _udp.transmit(pckt); }
    
    /// listen for UDPv6 packets on @port
    /// replaces existing listeners on the same port
    void udp6_listen(UDPv6::port_t port, UDPv6::listener_t func)
    {
      _udp6.listen(port, func);
    }
    /// send an UDPv6 packet, hopefully (please dont lie!)
    std::shared_ptr<PacketUDP6> udp6_create(
                                            Ethernet::addr ether_dest, const IP6::addr& ip_dest, UDPv6::port_t port)
    {
      return _udp6.create(ether_dest, ip_dest, port);
    }
    
    /// send an UDPv6 packet, hopefully (please dont lie!)
    int udp6_send(std::shared_ptr<PacketUDP6> pckt)
    {
      return _udp6.transmit(pckt);
    }
    
    void ip6_ndp_discovery()
    {
      _icmp6.discover();
    }
    
    /** Bind an IP and a netmask to a given device. 
        The function expects the given device to exist.*/
    static void
    ifconfig(
             netdev nic,
             IP4::addr ip,
             IP4::addr netmask,
             IP6::addr ip6);
    
    inline static IP4::addr ip4(netdev nic)
    { return _ip4_list[nic]; }
    
    inline static IP6::addr ip6(netdev nic)
    { return _ip6_list[nic]; }
    
    static Inet& up()
    {
      if (_ip4_list.size() < 1)
        WARN("<Inet> Is bringing up an IP stack without any IP addresses");      
      static Inet instance;
      return instance;
    }
    
    //typedef delegate<int(uint8_t*,int)> upstream_delg;
    
    
  
  private:
    
    /** Physical routes. These map 1-1 with Dev:: interfaces. */
    static std::map<uint16_t,IP4::addr> _ip4_list;
    static std::map<uint16_t,IP4::addr> _netmask_list;
    static std::map<uint16_t,IP6::addr> _ip6_list;
    static std::map<uint16_t,Ethernet*> _ethernet_list;
    static std::map<uint16_t,Arp*> _arp_list;    
    
    // This is the actual stack
    IP4  _ip4;
    ICMPv4 _icmp;
    UDP  _udp;
    IP6    _ip6;
    ICMPv6 _icmp6;
    UDPv6  _udp6;
    
    
    /** Don't think we *want* copy construction.
        @todo: Fix this with a singleton or something.
    */
    Inet(Inet& UNUSED(cpy)) = delete;
    
    Inet(std::vector<IP4::addr> ips);
    
    /** Initialize. For now IP and mac is passed on to Ethernet and Arp.
        @todo For now, mac- and IP-addresses are hardcoded here. 
        They should be user-definable
    */
    Inet();
    
  };

}

#endif
