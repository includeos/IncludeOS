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

#include <vector>
#include <unordered_set>

#include "inet.hpp"
#include "ip4/arp.hpp"
#include "ip4/ip4.hpp"
#include "ip4/udp.hpp"
#include "ip4/icmp4.hpp"
#include "dns/client.hpp"
#include "tcp/tcp.hpp"
#include "super_stack.hpp"

namespace net {

  class DHClient;

  /** A complete IP4 network stack */
  class Inet4 : public Inet<IP4>{
  public:

    using Vip4_list = std::unordered_set<IP4::addr>;

    std::string ifname() const override
    { return nic_.device_name(); }

    MAC::Addr link_addr() override
    { return nic_.mac(); }

    hw::Nic& nic() override
    { return nic_; }

    IP4::addr ip_addr() override
    { return ip4_addr_; }

    IP4::addr netmask() override
    { return netmask_; }

    IP4::addr gateway() override
    { return gateway_; }

    IP4::addr dns() override
    { return dns_server; }

    IP4& ip_obj() override
    { return ip4_; }

    void cache_link_addr(IP4::addr ip, MAC::Addr mac) override
    { arp_.cache(ip, mac); }

    void flush_link_cache() override
    { arp_.flush_cache(); }

    void set_link_cache_flush_interval(std::chrono::minutes min) override
    { arp_.set_cache_flush_interval(min); }

    /** Get the TCP-object belonging to this stack */
    TCP& tcp() override { return tcp_; }

    /** Get the UDP-object belonging to this stack */
    UDP& udp() override { return udp_; }

    /** Get the ICMP-object belonging to this stack */
    ICMPv4& icmp() override { return icmp_; }

    /** Get the DHCP client (if any) */
    auto dhclient() { return dhcp_;  }

    /**
     *  Error report in accordance with RFC 1122
     *  An ICMP error message has been received - forward to transport layer (UDP or TCP)
    */
    void error_report(Error_type type, Error_code code, Packet_ptr orig_pckt) override;

    /**
     * Set the forwarding delegate used by this stack.
     * If set it will get all incoming packets not intended for this stack.
     */
    void set_forward_delg(Forward_delg fwd) override {
      ip4_.set_packet_forwarding(fwd);
    }

    /**
     * Assign a delegate that checks if we have a route to a given IP
     */
    void set_route_checker(Route_checker delg) override
    { arp_.set_proxy_policy(delg); }

    /**
     * Get the forwarding delegate used by this stack.
     */
    Forward_delg forward_delg() override
    { return ip4_.forward_delg(); }


    Packet_ptr create_packet() override {
      return nic_.create_packet(nic_.frame_offset_link());
    }

    /**
     * Provision an IP packet
     * @param proto : IANA protocol number.
     */
    IP4::IP_packet_ptr create_ip_packet(Protocol proto) override {
      auto raw = nic_.create_packet(nic_.frame_offset_link());
      auto ip_packet = static_unique_ptr_cast<IP4::IP_packet>(std::move(raw));
      ip_packet->init(proto);
      return ip_packet;
    }

    IP_packet_factory ip_packet_factory() override
    { return IP_packet_factory{this, &Inet4::create_ip_packet}; }

    /** MTU retreived from Nic on construction */
    uint16_t MTU() const override
    { return MTU_; }

    /**
     * @func  a delegate that provides a hostname and its address, which is 0 if the
     * name @hostname was not found. Note: Test with INADDR_ANY for a 0-address.
     **/
    void resolve(const std::string& hostname,
                 resolve_func<IP4>  func) override
    {
      dns_.resolve(this->dns_server, hostname, func);
    }

    void set_gateway(IP4::addr gateway) override
    {
      this->gateway_ = gateway;
    }

    void set_dns_server(IP4::addr server) override
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

    virtual void
    reset_config() override
    {
      this->ip4_addr_ = IP4::ADDR_ANY;
      this->gateway_ = IP4::ADDR_ANY;
      this->netmask_ = IP4::ADDR_ANY;
    }

    // register a callback for receiving signal on free packet-buffers
    virtual void
    on_transmit_queue_available(transmit_avail_delg del) override {
      tqa.push_back(del);
    }

    size_t transmit_queue_available() override {
      return nic_.transmit_queue_available();
    }

    size_t buffers_available() override {
      return nic_.buffers_available();
    }

    void force_start_send_queues() override;

    void move_to_this_cpu() override;

    int  get_cpu_id() const noexcept override {
      return this->cpu_id;
    }

    /** Return the stack on the given Nic */
    template <int N = 0>
    static auto&& stack()
    {
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
      if (timeout > 0.0)
          stack<N>().negotiate_dhcp(timeout, on_timeout);
      return stack<N>();
    }

    /** Add virtual IP4 address as loopback */
    const Vip4_list virtual_ips() const noexcept override
    { return vip4s_; }

    /** Check if IP4 address is virtual loopback */
    bool is_loopback(IP4::addr a) const override
    {
      return a.is_loopback()
        or vip4s_.find(a) != vip4s_.end();
    }

    /** Add IP4 address as virtual loopback */
    void add_vip(IP4::addr a) override
    {
      if (not is_loopback(a)) {
        INFO("Inet4", "Adding virtual IP address %s", a.to_string().c_str());
        vip4s_.emplace(a);
      }
    }

    /** Add IP4 address as virtual loopback */
    void remove_vip(IP4::addr a) override
    { vip4s_.erase(a); }

    IP4::addr get_source_addr(IP4::addr dest) override
    {

      if (dest.is_loopback())
        return {127,0,0,1};

      if (is_loopback(dest))
        return dest;

      return ip_addr();
    }

    bool is_valid_source(IP4::addr src) override
    { return is_loopback(src) or src == ip_addr(); }

    /** Initialize with ANY_ADDR */
    Inet4(hw::Nic& nic);

  private:

    void process_sendq(size_t);
    // delegates registered to get signalled about free packets
    std::vector<transmit_avail_delg> tqa;

    IP4::addr ip4_addr_;
    IP4::addr netmask_;
    IP4::addr gateway_;
    IP4::addr dns_server;

    Vip4_list vip4s_ = {{127,0,0,1}};

    // This is the actual stack
    hw::Nic& nic_;
    Arp    arp_;
    IP4    ip4_;
    ICMPv4 icmp_;
    UDP    udp_;
    TCP    tcp_;

    // we need this to store the cache per-stack
    DNSClient dns_;

    std::shared_ptr<net::DHClient> dhcp_{};

    int   cpu_id;
    const uint16_t MTU_;

    friend class Super_stack;
  };
}

#endif
