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
#include <hw/devices.hpp> // 107: auto& eth0 = Dev::eth(0);
#include <hw/nic.hpp>
#include "inet.hpp"
#include "ethernet/ethernet.hpp"
#include "ip4/arp.hpp"
#include "ip4/ip4.hpp"
#include "ip4/udp.hpp"
#include "ip4/icmpv4.hpp"
#include "dns/client.hpp"
#include "tcp/tcp.hpp"
#include <vector>
#include "super_stack.hpp"

namespace net {

  class DHClient;

  /** A complete IP4 network stack */
  class Inet4 : public Inet<IP4>{
  public:

    virtual std::string ifname() const override
    { return nic_.ifname(); }

    hw::MAC_addr link_addr() override
    { return nic_.mac(); }

    IP4::addr ip_addr() override
    { return ip4_addr_; }

    IP4::addr netmask() override
    { return netmask_; }

    IP4::addr gateway() override
    { return gateway_; }

    IP4& ip_obj() override
    { return ip4_; }

    /** Get the TCP-object belonging to this stack */
    TCP& tcp() override { return tcp_; }

    /** Get the UDP-object belonging to this stack */
    UDP& udp() override { return udp_; }

    /** Get the DHCP client (if any) */
    auto dhclient() { return dhcp_;  }

    /**
     * Set the forwarding delegate used by this stack.
     * If set it will get all incoming packets not intended for this stack.
     */
    void set_forward_delg(Forward_delg fwd) override { forward_packet_ = fwd; }

    /**
     * Get the forwarding delegate used by this stack.
     */
    Forward_delg forward_delg() override { return forward_packet_; }

    /** Create a Packet, with a preallocated buffer.
        @param size : the "size" reported by the allocated packet.
    */
    virtual Packet_ptr create_packet(size_t size) override {
      // get buffer (as packet + data)
      auto* ptr = (Packet*) bufstore_.get_buffer();
      // place packet at front of buffer
      new (ptr) Packet(nic_.bufsize(), size, &bufstore_);
      // regular shared_ptr that calls delete on Packet
      return Packet_ptr(ptr);
    }

    /** MTU retreived from Nic on construction */
    virtual uint16_t MTU() const override
    { return MTU_; }

    /**
     * @func  a delegate that provides a hostname and its address, which is 0 if the
     * name @hostname was not found. Note: Test with INADDR_ANY for a 0-address.
     **/
    virtual void resolve(const std::string& hostname,
                         resolve_func<IP4>  func) override
    {
      dns.resolve(this->dns_server, hostname, func);
    }

    virtual void set_gateway(IP4::addr gateway) override
    {
      this->gateway_ = gateway;
    }

    virtual void set_dns_server(IP4::addr server) override
    {
      this->dns_server = server;
    }

    /**
     * @brief Try to negotiate DHCP
     * @details Initialize DHClient if not present and tries to negotitate dhcp.
     * Also takes an optional timeout parameter and optional timeout function.
     *
     * @param timeout number of seconds before request should timeout
     * @param dhcp_timeout_func DHCP timeout handler
     */
    void negotiate_dhcp(double timeout = 10.0, dhcp_timeout_func = nullptr) override;

    // handler called after the network successfully, or
    // unsuccessfully negotiated with DHCP-server
    // the timeout parameter indicates whether dhcp negotitation failed
    void on_config(dhcp_timeout_func handler);

    /** We don't want to copy or move an IP-stack. It's tied to a device. */
    Inet4(Inet4&) = delete;
    Inet4(Inet4&&) = delete;
    Inet4& operator=(Inet4) = delete;
    Inet4 operator=(Inet4&&) = delete;

    virtual void
    network_config(IP4::addr addr, IP4::addr nmask, IP4::addr gateway, IP4::addr dns = IP4::ADDR_ANY) override
    {
      this->ip4_addr_  = addr;
      this->netmask_   = nmask;
      this->gateway_    = gateway;
      this->dns_server = (dns == IP4::ADDR_ANY) ? gateway : dns;
      INFO("Inet4", "Network configured");
      INFO2("IP: \t\t%s", ip4_addr_.str().c_str());
      INFO2("Netmask: \t%s", netmask_.str().c_str());
      INFO2("Gateway: \t%s", gateway_.str().c_str());
      INFO2("DNS Server: \t%s", dns_server.str().c_str());
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

    /** Return the stack on the given Nic */
    template <int N = 0>
    static auto&& stack()
    {
      //static Inet4 inet{hw::Devices::nic(N)};
      return Super_stack::get<IP4>(N);
    }

    /** Static IP config */
    template <int N = 0>
    static auto&& ifconfig(
      IP4::addr addr,
      IP4::addr nmask,
      IP4::addr gateway,
      IP4::addr dns = IP4::ADDR_ANY)
    {
      stack<N>().network_config(addr, nmask, gateway, dns);
      return stack<N>();
    }

    /** DHCP config */
    template <int N = 0>
    static auto& ifconfig(double timeout = 10.0, dhcp_timeout_func on_timeout = nullptr)
    {
      stack<N>().negotiate_dhcp(timeout, on_timeout);
      return stack<N>();
    }


    /** A super stack */
    friend class Super_stack;


  private:

    /** Initialize with ANY_ADDR */
    Inet4(hw::Nic& nic);

    void process_sendq(size_t);
    // delegates registered to get signalled about free packets
    std::vector<transmit_avail_delg> tqa;

    IP4::addr ip4_addr_;
    IP4::addr netmask_;
    IP4::addr gateway_;
    IP4::addr dns_server;

    // This is the actual stack
    hw::Nic& nic_;
    Arp arp_;
    IP4  ip4_;
    ICMPv4 icmp_;
    UDP  udp_;
    TCP tcp_;
    Forward_delg forward_packet_;
    // we need this to store the cache per-stack
    DNSClient dns;

    std::shared_ptr<net::DHClient> dhcp_{};
    BufferStore& bufstore_;

    const uint16_t MTU_;
  };
}

#endif
