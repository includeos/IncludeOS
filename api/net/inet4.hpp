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

#ifndef NET_INET4_HPP
#define NET_INET4_HPP

#include <kernel/syscalls.hpp> // panic()
#include <hw/dev.hpp> // 107: auto& eth0 = Dev::eth(0);
#include <hw/nic.hpp>
#include "inet.hpp"
#include "ethernet.hpp"
#include "ip4/arp.hpp"
#include "ip4/ip4.hpp"
#include "ip4/udp.hpp"
#include "dns/client.hpp"
#include "tcp.hpp"
#include "dhcp/dh4client.hpp"
#include <vector>

#include "ip4/icmpv4.hpp"

namespace net {

  /** A complete IP4 network stack */
  template <typename DRIVER>
  class Inet4 : public Inet<Ethernet, IP4>{
  public:

    Ethernet::addr link_addr() override
    { return eth_.mac(); }

    IP4::addr ip_addr() override
    { return ip4_addr_; }

    IP4::addr netmask() override
    { return netmask_; }

    IP4::addr router() override
    { return router_; }

    Ethernet& link() override
    { return eth_; }

    IP4& ip_obj() override
    { return ip4_; }

    /** Get the TCP-object belonging to this stack */
    TCP& tcp() override { return tcp_; }

    /** Get the UDP-object belonging to this stack */
    UDP& udp() override { return udp_; }

    /** Get the DHCP client (if any) */
    inline std::shared_ptr<DHClient> dhclient() override { return dhcp_;  }

    /** Create a Packet, with a preallocated buffer.
        @param size : the "size" reported by the allocated packet.
        @note as of v0.6.3 this has no effect other than to force the size to be
        set explicitly by the caller.
        @todo make_shared will allocate with new. This is fast in IncludeOS,
        (no context switch for sbrk) but consider overloading operator new.
    */
    inline Packet_ptr createPacket(size_t size) override {
      // Create a release delegate, for returning buffers
      auto release = BufferStore::release_del::from
        <BufferStore, &BufferStore::release_offset_buffer>(nic_.bufstore());
      // Create the packet, using  buffer and .
      return std::make_shared<Packet>(bufstore_.get_offset_buffer(),
                                      bufstore_.offset_bufsize(), size, release);
    }

    // We have to ask the Nic for the MTU
    virtual inline uint16_t MTU() const override
    { return nic_.MTU(); }

    inline auto available_capacity()
    { return bufstore_.capacity(); }

    /**
     * @func  a delegate that provides a hostname and its address, which is 0 if the
     * name @hostname was not found. Note: Test with INADDR_ANY for a 0-address.
     **/
    inline virtual void
    resolve(const std::string& hostname,
            resolve_func<IP4>  func) override
    {
      dns.resolve(this->dns_server, hostname, func);
    }

    inline virtual void
    set_dns_server(IP4::addr server) override
    {
      this->dns_server = server;
    }

    /** We don't want to copy or move an IP-stack. It's tied to a device. */
    Inet4(Inet4&) = delete;
    Inet4(Inet4&&) = delete;
    Inet4& operator=(Inet4) = delete;
    Inet4 operator=(Inet4&&) = delete;

    /** Initialize with static IP / netmask */
    Inet4(hw::Nic<DRIVER>& nic, IP4::addr ip, IP4::addr netmask);

    /** Initialize with DHCP  */
    Inet4(hw::Nic<DRIVER>& nic);

    virtual void
    network_config(IP4::addr addr, IP4::addr nmask, IP4::addr router, IP4::addr dns) override
    {
      INFO("Inet4", "Reconfiguring network. New IP: %s", addr.str().c_str());
      this->ip4_addr_  = addr;
      this->netmask_   = nmask;
      this->router_    = router;
      this->dns_server = dns;
    }

    // register a callback for receiving signal on free packet-buffers
    virtual void
    on_transmit_queue_available(transmit_avail_delg del) override {
      tqa.push_back(del);
      printf("* adding transmit listener  (sz=%u)\n", tqa.size() );
    }

    virtual size_t transmit_queue_available() override {
      return nic_.transmit_queue_available();
    }

    inline virtual size_t buffers_available() override {
      return nic_.buffers_available();
    }

  private:
    inline void process_sendq(size_t);
    // delegates registered to get signalled about free packets
    std::vector<transmit_avail_delg> tqa;
    
    IP4::addr ip4_addr_;
    IP4::addr netmask_;
    IP4::addr router_;
    IP4::addr dns_server;

    // This is the actual stack
    hw::Nic<DRIVER>& nic_;
    Ethernet eth_;
    Arp arp_;
    IP4  ip4_;
    ICMPv4 icmp_;
    UDP  udp_;
    TCP tcp_;
    // we need this to store the cache per-stack
    DNSClient dns;

    std::shared_ptr<net::DHClient> dhcp_{};
    BufferStore& bufstore_;
  };
}

#include "inet4.inc"

#endif
