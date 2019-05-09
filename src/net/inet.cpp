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

#include <net/inet>
#include <net/dhcp/dh4client.hpp>
#include <net/ip6/slaac.hpp>
#include <smp>
#include <net/socket.hpp>
#include <net/tcp/packet4_view.hpp> // due to ICMP error //temp
#include <net/udp/packet4_view.hpp> // due to ICMP error //temp

using namespace net;

Inet::Inet(hw::Nic& nic)
  : dns_server_(IP4::ADDR_ANY),
    nic_(nic), arp_(*this), ndp_(*this),
    mld_(*this), /*mld2_(*this),*/
    ip4_(*this), ip6_(*this),
    icmp_(*this), icmp6_(*this),
    udp_(*this), tcp_(*this),
    dns_(*this), domain_name_{}, MTU_(nic.MTU())
{
  static_assert(sizeof(ip4::Addr) == 4, "IPv4 addresses must be 32-bits");

  /** SMP related **/
  this->cpu_id = SMP::cpu_id();
  INFO("Inet", "Bringing up %s on CPU %d",
        ifname().c_str(), this->get_cpu_id());

  /** Upstream delegates */
  auto arp_bottom(upstream{arp_, &Arp::receive});
  auto ip4_bottom(upstream_ip{ip4_, &IP4::receive});
  auto ip6_bottom(upstream_ip{ip6_, &IP6::receive});
  auto icmp4_bottom(upstream{icmp_, &ICMPv4::receive});
  auto icmp6_bottom(upstream{icmp6_, &ICMPv6::receive});
  auto udp4_bottom(upstream{udp_, &UDP::receive4});
  auto udp6_bottom(upstream{udp_, &UDP::receive6});
  auto tcp4_bottom(upstream{tcp_, &TCP::receive4});
  auto tcp6_bottom(upstream{tcp_, &TCP::receive6});
  auto ndp_bottom(upstream{ndp_, &Ndp::receive});
  auto mld_bottom(upstream{mld_, &Mld::receive});

  /** Upstream wiring  */
  // Packets available
  nic.on_transmit_queue_available({this, &Inet::process_sendq});

  // Link -> Arp
  nic_.set_arp_upstream(arp_bottom);

  // Link -> IP4
  nic_.set_ip4_upstream(ip4_bottom);

  // Link -> IP6
  nic_.set_ip6_upstream(ip6_bottom);

  // IP4 -> ICMP
  ip4_.set_icmp_handler(icmp4_bottom);

  // IP6 -> ICMP6
  ip6_.set_icmp_handler(icmp6_bottom);

  // IP4 -> UDP
  ip4_.set_udp_handler(udp4_bottom);

  // IP6 -> UDP
  ip6_.set_udp_handler(udp6_bottom);

  // IP4 -> TCP
  ip4_.set_tcp_handler(tcp4_bottom);

  // IP6 -> TCP
  ip6_.set_tcp_handler(tcp6_bottom);

  // ICMPv6 -> NDP
  icmp6_.set_ndp_handler(ndp_bottom);

  // ICMPv6 -> MLD
  icmp6_.set_mld_handler(mld_bottom);

  /** Downstream delegates */
  auto link_top(nic_.create_link_downstream());
  auto arp_top(IP4::downstream_arp{arp_, &Arp::transmit});
  auto ndp_top(IP6::downstream_ndp{ndp_, &Ndp::transmit});
  auto ip4_top(downstream{ip4_, &IP4::transmit});
  auto ip6_top(downstream{ip6_, &IP6::transmit});

  /** Downstream wiring. */

  // ICMP -> IP4
  icmp_.set_network_out(ip4_top);

  // ICMPv6 -> IP6
  icmp6_.set_network_out(ip6_top);

  // UDP4 -> IP4
  udp_.set_network_out4(ip4_top);

  // UDP6 -> IP6
  udp_.set_network_out6(ip6_top);

  // TCP4 -> IP4
  tcp_.set_network_out4(ip4_top);

  // TCP6 -> IP6
  tcp_.set_network_out6(ip6_top);

  // IP4 -> Arp
  ip4_.set_linklayer_out(arp_top);

  // IP6 -> Ndp
  ip6_.set_linklayer_out(ndp_top);

  // NDP -> Link
  ndp_.set_linklayer_out(link_top);

  // MLD -> Link
  mld_.set_linklayer_out(link_top);

  // Arp -> Link
  assert(link_top);
  arp_.set_linklayer_out(link_top);

#ifndef INCLUDEOS_SINGLE_THREADED
  // move this nework stack automatically
  // to the current CPU if its not 0
  if (SMP::cpu_id() != 0) {
    this->move_to_this_cpu();
  }
#endif
}

void Inet::error_report(Error& err, Packet_ptr orig_pckt) {
  // if its a forged packet, it might be too small
  if (orig_pckt->size() < 40) return;

  auto pckt_ip4 = static_unique_ptr_cast<PacketIP4>(std::move(orig_pckt));
  // Get the destination to the original packet
  Socket dest = [](std::unique_ptr<PacketIP4>& pkt)->Socket {
    // if its a forged packet, it might not be IPv4
    if (pkt->is_ipv4() == false) return {};
    switch (pkt->ip_protocol()) {
      case Protocol::UDP: {
        auto udp = udp::Packet4_view_raw(pkt.get());
        return udp.destination();
      }
      case Protocol::TCP: {
        auto tcp = tcp::Packet4_view_raw(pkt.get());
        return tcp.destination();
      }
      default:
        return {};
    }
  }(pckt_ip4);

  bool too_big = false;
  if (err.is_icmp()) {
    auto* icmp_err = dynamic_cast<ICMP_error*>(&err);
    if (icmp_err == nullptr) {
      return; // not an ICMP error
    }

    if (icmp_err->is_too_big()) {
      // If Path MTU Discovery is not enabled, ignore the ICMP Datagram Too Big message
      if (not ip4_.path_mtu_discovery())
        return;

      too_big = true;

      // We have received a response to a packet with an MTU that is too big for a node in the path,
      // and the packet has been dropped (the original packet was too big and the Don't Fragment bit was set)

      // Notify every protocol of the received MTU if any of the protocol's connections use the given
      // path (based on destination address)

      // Also need to notify the instance that sent the packet that the packet has been dropped, so
      // it can retransmit it

      // A Destination Unreachable: Fragmentation Needed ICMP error message has been received
      // And we'll notify the IP layer of the received MTU value
      // IP will create the path if it doesn't exist and only update the value if
      // the value is smaller than the already registered pmtu for this path/destination
      // If the received MTU value is zero, the method will use the original packet's Total Length
      // and Header Length values to estimate a new Path MTU value
      ip4_.update_path(dest, icmp_err->pmtu(), too_big, pckt_ip4->ip_total_length(), pckt_ip4->ip_header_length());

      // The actual MTU for the path is set in the error object
      icmp_err->set_pmtu(ip4_.pmtu(dest));
    }
  }

  if (too_big) {
    // Notify both transport layers in case they use the path
    udp_.error_report(err, dest);
    tcp_.error_report(err, dest);
  } else if (pckt_ip4->ip_protocol() == Protocol::UDP) {
    udp_.error_report(err, dest);
  } else if (pckt_ip4->ip_protocol() == Protocol::TCP) {
    tcp_.error_report(err, dest);
  }
}

void Inet::negotiate_dhcp(double timeout, dhcp_timeout_func handler) {
  INFO("Inet", "Negotiating DHCP (%.1fs timeout)...", timeout);
  if (!dhcp_)
      dhcp_ = std::make_shared<DHClient>(*this);
  // @timeout for DHCP-server negotation
  dhcp_->negotiate(std::chrono::seconds((uint32_t)timeout));
  // add timeout_handler if supplied
  if (handler)
      dhcp_->on_config(handler);
}

void Inet::autoconf_v6(int retries, slaac_timeout_func handler,
        uint64_t token, bool use_token)
{

  INFO("Inet6", "Attempting automatic configuration of IPv6 address");
  if (!slaac_)
    slaac_ = std::make_unique<Slaac>(*this);

  // @Retries for Slaac auto-configuration
  slaac_->autoconf_start(retries, token, use_token);

  // add failure_handler if supplied
  if (handler)
    slaac_->on_config(handler);
}

void Inet::network_config(ip4::Addr addr,
                           ip4::Addr nmask,
                           ip4::Addr gw,
                           ip4::Addr dns)
{
  ip_obj().set_addr(addr);
  ip_obj().set_netmask(nmask);
  ip_obj().set_gateway(gw);
  this->dns_server_ = (dns == IP4::ADDR_ANY) ? gw : dns;
  INFO("Inet", "Network configured (%s)", nic_.mac().to_string().c_str());
  INFO2("IP: \t\t%s", ip_addr().str().c_str());
  INFO2("Netmask: \t%s", netmask().str().c_str());
  INFO2("Gateway: \t%s", gateway().str().c_str());
  INFO2("DNS Server: \t%s", dns_server_.str().c_str());

  for(auto& handler : configured_handlers_)
    handler(*this);

  configured_handlers_.clear();
}

void Inet::network_config6(IP6::addr addr6,
                           uint8_t   prefix6,
                           IP6::addr gateway6)
{

  ndp().set_static_addr(std::move(addr6));
  ndp().set_static_prefix(prefix6);
  ndp().set_static_gateway(std::move(gateway6));
  INFO("Inet6", "Network configured (%s)", nic_.mac().to_string().c_str());
  INFO2("IP6: \t\t%s", ndp().static_ip().to_string().c_str());
  INFO2("Prefix: \t%d", ndp().static_prefix());
  INFO2("Gateway: \t%s", ndp().static_gateway().str().c_str());

  for(auto& handler : configured_handlers_)
    handler(*this);

  configured_handlers_.clear();
}

void Inet::add_addr(const ip6::Addr& addr, uint8_t prefix,
              uint32_t pref_lifetime, uint32_t valid_lifetime)
{
  Expects(prefix != 0);
  int r = this->ip6_.addr_list().input(
    addr, prefix, pref_lifetime, valid_lifetime);
  if(r == 1) {
    INFO("Inet6", "Address configured %s/%u", addr.to_string().c_str(), prefix);
  }
}

void Inet::add_addr_autoconf(const ip6::Addr& addr, uint8_t prefix,
                       uint32_t pref_lifetime, uint32_t valid_lifetime)
{
  Expects(prefix == 64);
  int r = this->ip6_.addr_list().input_autoconf(
    addr, prefix, pref_lifetime, valid_lifetime);
  if(r == 1) {
    INFO("Inet6", "Address configured %s (autoconf)", addr.to_string().c_str());
  }
}

void Inet::enable_conntrack(std::shared_ptr<Conntrack> ct)
{
  Expects(conntrack_ == nullptr && "Conntrack is already set");
  conntrack_ = ct;
}

void Inet::process_sendq(size_t packets) {

  ////////////////////////////////////////////
  // divide up fairly
  size_t div = packets / tqa.size();

  // give each protocol a chance to take
  for (size_t i = 0; i < tqa.size(); i++)
    tqa[i](div);

  // hand out remaining
  for (size_t i = 0; i < tqa.size(); i++) {
    div = transmit_queue_available();
    if (!div) break;
    // give as much as possible
    tqa[i](div);
  }
  ////////////////////////////////////////////

  /*
    size_t list[tqa.size()];
    for (size_t i = 0; i < tqa.size(); i++)
    list[i] = tqa[i](0);

    size_t give[tqa.size()] = {0};
    int cp = 0; // current protocol

    // distribute packets one at a time for each protocol
    while (packets--)
    {
    if (list[cp])
    {
    // give one packet
    give[cp]++;  list[cp]--;
    }
    cp = (cp + 1) % tqa.size();
    }
    // hand out several packets per protocol
    for (size_t i = 0; i < tqa.size(); i++)
    if (give[i]) tqa[i](give[i]);
  */
}

void Inet::force_start_send_queues()
{
  size_t packets = transmit_queue_available();
  if (packets) process_sendq(packets);
}

void Inet::move_to_this_cpu()
{
  this->cpu_id = SMP::cpu_id();
  nic_.move_to_this_cpu();
}

void Inet::cache_link_addr(ip4::Addr ip, MAC::Addr mac)
{ arp_.cache(ip, mac); }

void Inet::flush_link_cache()
{ arp_.flush_cache(); }

void Inet::set_link_cache_flush_interval(std::chrono::minutes min)
{ arp_.set_cache_flush_interval(min); }

void Inet::set_route_checker(Route_checker delg)
{ arp_.set_proxy_policy(delg); }

void Inet::reset_pmtu(Socket dest, IP4::PMTU pmtu)
{ tcp_.reset_pmtu(dest, pmtu); /* Maybe later: udp_.reset_pmtu(dest, pmtu);*/ }

void Inet::resolve(const std::string& hostname,
     resolve_func       func,
     bool               force)
{
  if(is_configured_v6() and dns_server6_ != ip6::Addr::addr_any)
    resolve(hostname, this->dns_server6_, func, force);
  else
    resolve(hostname, this->dns_server_, func, force);
}

void Inet::resolve(const std::string& hostname,
      net::Addr         server,
      resolve_func      func,
      bool              force)
{
  Expects(not hostname.empty());
  dns_.resolve(server, hostname, func, force);
}

void Inet::set_route_checker6(Route_checker6 delg)
{ ndp_.set_proxy_policy(delg); }
