#ifndef NET_INET4_HPP
#define NET_INET4_HPP

#include <syscalls.hpp> // panic()
#include <dev.hpp> // 107: auto& eth0 = Dev::eth(0);
#include <net/inet.hpp>
#include <net/ethernet.hpp>
#include <net/arp.hpp>
#include <net/ip4.hpp>
#include <net/icmp.hpp>
#include "ip4/udp.hpp"
#include <net/tcp.hpp>

#include <vector>

#include <nic.hpp>

namespace net {
     
  /** A complete IP4 network stack */
  template <typename DRIVER>
  class Inet4 : public Inet<Ethernet, IP4>{
  public:
    
    inline const Ethernet::addr& link_addr() override 
    { return eth_.mac(); }
    
    inline Ethernet& link() override
    { return eth_; }    
    
    inline const IP4::addr& ip_addr() override 
    { return ip4_addr_; }

    inline const IP4::addr& netmask() override 
    { return netmask_; }
    
    inline IP4& ip_obj() override
    { return ip4_; }
    
    /** Get the TCP-object belonging to this stack */
    inline TCP& tcp() override { debug("<TCP> Returning tcp-reference to %p \n",&_tcp); return tcp_; }        
    
    /** Create a Packet, with a preallocated buffer.
	@param size : the "size" reported by the allocated packet. 
	@note as of v0.6.3 this has no effect other than to force the size to be
	set explicitly by the caller.
    */
    inline Packet_ptr createPacket(size_t size) override {       
      return std::make_shared<Packet>(bufstore_.get_offset_buffer(), 
				      bufstore_.offset_bufsize(), size, 
				      BufferStore::release_del::from<BufferStore, &BufferStore::release_offset_buffer> (nic_.bufstore()));
    }
    
    inline UDP& udp() override { return udp_; };
    
    /** Don't think we *want* copy construction.
	@todo: Fix this with a singleton or something.
   */
    Inet4(Inet4&) = delete;
    Inet4(Inet4&&) = delete;
    
    
    /** Initialize. For now IP and mac is passed on to Ethernet and Arp.
	@todo For now, mac- and IP-addresses are hardcoded here. 
	They should be user-definable
    */
    Inet4(Nic<DRIVER>& nic, IP4::addr ip, IP4::addr netmask); 
    
  private:
    
    const IP4::addr ip4_addr_;
    const IP4::addr netmask_;
    
    // This is the actual stack
    Nic<DRIVER>& nic_;
    Ethernet eth_;
    Arp arp_;
    IP4  ip4_;
    ICMP icmp_;
    UDP  udp_;
    TCP tcp_;

    // We have to ask the Nic for the MTU
    uint16_t MTU = 0;
    
    BufferStore& bufstore_;

  };
}

#include "inet4.inc"

#endif
