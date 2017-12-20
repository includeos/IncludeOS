// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015-2017 Oslo and Akershus University College of Applied Sciences
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

    using Vip4_list = Vip_list;

    std::string ifname() const override
    { return nic_.device_name(); }

    MAC::Addr link_addr() const override
    { return nic_.mac(); }

    hw::Nic& nic() override
    { return nic_; }

    IP4::addr ip_addr() const override
    { return ip4_addr_; }

    IP4::addr netmask() const override
    { return netmask_; }

    IP4::addr gateway() const override
    { return gateway_; }

    IP4::addr dns_addr() const override
    { return dns_server_; }

    IP4::addr broadcast_addr() const override
    { return ip4_addr_ | ( ~ netmask_); }

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
     * @brief      Disable or enable Path MTU Discovery (enabled by default)
     *             RFC 1191
     *             If enabled, it sets the Don't Fragment flag on each IP4 packet
     *             TCP and UDP acts based on this being enabled or not
     *
     * @param[in]  on    Enables Path MTU Discovery if true, disables if false
     * @param[in]  aged  Number of minutes that indicate that a PMTU value
     *                   has grown stale and should be reset/increased
     *                   This could be set to "infinity" (PMTU should never be
     *                   increased) by setting the value to IP4::INFINITY
     */
    void set_path_mtu_discovery(bool on, uint16_t aged = 10) override
    { ip4_.set_path_mtu_discovery(on, aged); }

    /**
     * @brief      Triggered by IP when a Path MTU value has grown stale and the value
     *             is reset (increased) to check if the PMTU for the path could have increased
     *             This is NOT a change in the Path MTU in response to receiving an ICMP Too Big message
     *             and no retransmission of packets should take place
     *
     * @param[in]  dest  The destination/path
     * @param[in]  pmtu  The reset PMTU value
     */
    void reset_pmtu(Socket dest, IP4::PMTU pmtu) override
    { tcp_.reset_pmtu(dest, pmtu); /* Maybe later: udp_.reset_pmtu(dest, pmtu);*/ }

    /**
     *  Error reporting
     *
     *  Including ICMP error report in accordance with RFC 1122 and handling of ICMP
     *  too big messages in accordance with RFC 1191, 1981 and 4821 (Path MTU Discovery
     *  and Packetization Layer Path MTU Discovery)
     *
     *  Forwards errors to the transport layer (UDP and TCP)
    */
    void error_report(Error& err, Packet_ptr orig_pckt) override;

    /**
     * Set the forwarding delegate used by this stack.
     * If set it will get all incoming packets not intended for this stack.
     * NOTE: This delegate is expected to call the forward chain
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
                 resolve_func<IP4>  func,
                 bool               force = false) override
    {
      dns_.resolve(this->dns_server_, hostname, func, force);
    }

    void resolve(const std::string& hostname,
                  IP4::addr         server,
                  resolve_func<IP4> func,
                  bool              force = false) override
    {
      dns_.resolve(server, hostname, func, force);
    }

    void set_domain_name(std::string domain_name) override
    { this->domain_name_ = std::move(domain_name); }

    const std::string& domain_name() const override
    { return this->domain_name_; }

    void set_gateway(IP4::addr gateway) override
    {
      this->gateway_ = gateway;
    }

    void set_dns_server(IP4::addr server) override
    {
      this->dns_server_ = server;
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

    bool is_configured() const override
    {
      return ip4_addr_ != 0;
    }

    // handler called after the network is configured,
    // either by DHCP or static network configuration
    void on_config(on_configured_func handler) override
    {
      configured_handlers_.push_back(handler);
    }

    /** We don't want to copy or move an IP-stack. It's tied to a device. */
    Inet4(Inet4&) = delete;
    Inet4(Inet4&&) = delete;
    Inet4& operator=(Inet4) = delete;
    Inet4 operator=(Inet4&&) = delete;

    void network_config(IP4::addr addr,
                        IP4::addr nmask,
                        IP4::addr gateway,
                        IP4::addr dns = IP4::ADDR_ANY) override;

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
    size_t buffers_total() override {
      return nic_.buffers_total();
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
        or std::find( vip4s_.begin(), vip4s_.end(), a) != vip4s_.end();
    }

    /** Add IP4 address as virtual loopback */
    void add_vip(IP4::addr a) override
    {
      if (not is_loopback(a)) {
        INFO("Inet4", "Adding virtual IP address %s", a.to_string().c_str());
        vip4s_.emplace_back(a);
      }
    }

    /** Add IP4 address as virtual loopback */
    void remove_vip(IP4::addr a) override
    {
      auto it = std::find(vip4s_.begin(), vip4s_.end(), a);
      if (it != vip4s_.end())
        vip4s_.erase(it);
    }

    IP4::addr get_source_addr(IP4::addr dest) override
    {

      if (dest.is_loopback())
        return {127,0,0,1};

      if (is_loopback(dest))
        return dest;

      return ip_addr();
    }

    bool is_valid_source(IP4::addr src) const override
    { return src == ip_addr() or is_loopback(src); }

    virtual std::shared_ptr<Conntrack>& conntrack() override
    { return conntrack_; }

    virtual const std::shared_ptr<Conntrack>& conntrack() const override
    { return conntrack_; }

    virtual void enable_conntrack(std::shared_ptr<Conntrack> ct) override;

    virtual Port_utils& tcp_ports() override
    { return tcp_ports_; }

    virtual Port_utils& udp_ports() override
    { return udp_ports_; }

    /** Initialize with ANY_ADDR */
    Inet4(hw::Nic& nic);

  private:

    void process_sendq(size_t);
    // delegates registered to get signalled about free packets
    std::vector<transmit_avail_delg> tqa;

    IP4::addr ip4_addr_;
    IP4::addr netmask_;
    IP4::addr gateway_;
    IP4::addr dns_server_;

    Vip4_list vip4s_ = {{127,0,0,1}};

    // This is the actual stack
    hw::Nic& nic_;
    Arp    arp_;
    IP4    ip4_;
    ICMPv4 icmp_;
    UDP    udp_;
    TCP    tcp_;

    Port_utils tcp_ports_;
    Port_utils udp_ports_;

    std::shared_ptr<Conntrack> conntrack_;

    // we need this to store the cache per-stack
    DNSClient dns_;
    std::string domain_name_;

    std::shared_ptr<net::DHClient> dhcp_{};

    std::vector<on_configured_func> configured_handlers_;

    int   cpu_id;
    const uint16_t MTU_;

    friend class Super_stack;
  };
}

#endif
