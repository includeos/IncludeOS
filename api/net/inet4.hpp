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
#include "ip4/icmpv4.hpp"
#include "dns/client.hpp"
#include "tcp.hpp"
#include <vector>

namespace net {

  class DHClient;

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
    auto dhclient() { return dhcp_;  }

    /** Create a Packet, with a preallocated buffer.
        @param size : the "size" reported by the allocated packet.
        @note as of v0.6.3 this has no effect other than to force the size to be
        set explicitly by the caller.
        @todo make_shared will allocate with new. This is fast in IncludeOS,
        (no context switch for sbrk) but consider overloading operator new.
    */
    virtual Packet_ptr createPacket(size_t size) override {

      // get buffer (as packet + data)
      auto* ptr = (Packet*) bufstore_.get_buffer();
      // place packet at front of buffer
      new (ptr) Packet(nic_.bufsize(), size);
      // shared_ptr with custom deleter
      return std::shared_ptr<Packet>(ptr, 
          [this] (Packet* p) { bufstore_.release((uint8_t*) p); });
    }

    // We have to ask the Nic for the MTU
    virtual uint16_t MTU() const override
    { return nic_.MTU(); }

    /**
     * @func  a delegate that provides a hostname and its address, which is 0 if the
     * name @hostname was not found. Note: Test with INADDR_ANY for a 0-address.
     **/
    virtual void resolve(const std::string& hostname,
                         resolve_func<IP4>  func) override
    {
      dns.resolve(this->dns_server, hostname, func);
    }

    virtual void set_dns_server(IP4::addr server) override
    {
      this->dns_server = server;
    }

    // handler called after the network successfully, or
    // unsuccessfully negotiated with DHCP-server
    // the timeout parameter indicates whether dhcp negotitation failed
    void on_config(delegate<void(bool)> handler);

    /** We don't want to copy or move an IP-stack. It's tied to a device. */
    Inet4(Inet4&) = delete;
    Inet4(Inet4&&) = delete;
    Inet4& operator=(Inet4) = delete;
    Inet4 operator=(Inet4&&) = delete;

    /** Initialize with static IP / netmask */
    Inet4(hw::Nic<DRIVER>& nic, IP4::addr ip, IP4::addr netmask);

    /** Initialize with DHCP  */
    Inet4(hw::Nic<DRIVER>& nic, double timeout = 10.0);

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
    }

    virtual size_t transmit_queue_available() override {
      return nic_.transmit_queue_available();
    }

    virtual size_t buffers_available() override {
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

namespace net {
  template <int N = 0, typename Driver = VirtioNet>
  inline auto new_ipv4_stack(const double timeout, delegate<void(bool)> handler)
  {
    auto& eth = hw::Dev::eth<N,Driver>();
    auto inet = std::make_unique<net::Inet4<Driver>>(eth, timeout);
    inet->on_config(handler);
    return inet;
  }

  template <int N = 0, typename Driver = VirtioNet>
  inline auto new_ipv4_stack(const IP4::addr addr, const IP4::addr nmask, const IP4::addr router,
                             const IP4::addr dns = { 8,8,8,8 })
  {
    auto& eth = hw::Dev::eth<N,Driver>();
    auto inet = std::make_unique<net::Inet4<Driver>>(eth);
    inet->network_config(addr, nmask, router, dns);
    return inet;
  }
} //< namespace net

#endif
