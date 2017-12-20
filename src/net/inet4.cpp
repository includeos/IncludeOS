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

#include <net/inet4.hpp>
#include <net/dhcp/dh4client.hpp>
#include <smp>
#include <net/socket.hpp>

using namespace net;

Inet4::Inet4(hw::Nic& nic)
  : ip4_addr_(IP4::ADDR_ANY),
    netmask_(IP4::ADDR_ANY),
    gateway_(IP4::ADDR_ANY),
    dns_server_(IP4::ADDR_ANY),
    nic_(nic), arp_(*this), ip4_(*this),
    icmp_(*this), udp_(*this), tcp_(*this), dns_(*this),
    domain_name_{},
    MTU_(nic.MTU())
{
  static_assert(sizeof(IP4::addr) == 4, "IPv4 addresses must be 32-bits");

  /** SMP related **/
  this->cpu_id = SMP::cpu_id();
  INFO("Inet4", "Bringing up %s on CPU %d",
        ifname().c_str(), this->get_cpu_id());

  /** Upstream delegates */
  auto arp_bottom(upstream{arp_, &Arp::receive});
  auto ip4_bottom(upstream_ip{ip4_, &IP4::receive});
  auto icmp4_bottom(upstream{icmp_, &ICMPv4::receive});
  auto udp4_bottom(upstream{udp_, &UDP::receive});
  auto tcp_bottom(upstream{tcp_, &TCP::receive});

  /** Upstream wiring  */
  // Packets available
  nic.on_transmit_queue_available({this, &Inet4::process_sendq});

  // Link -> Arp
  nic_.set_arp_upstream(arp_bottom);

  // Link -> IP4
  nic_.set_ip4_upstream(ip4_bottom);

  // IP4 -> ICMP
  ip4_.set_icmp_handler(icmp4_bottom);

  // IP4 -> UDP
  ip4_.set_udp_handler(udp4_bottom);

  // IP4 -> TCP
  ip4_.set_tcp_handler(tcp_bottom);

  /** Downstream delegates */
  auto link_top(nic_.create_link_downstream());
  auto arp_top(IP4::downstream_arp{arp_, &Arp::transmit});
  auto ip4_top(downstream{ip4_, &IP4::transmit});

  /** Downstream wiring. */

  // ICMP -> IP4
  icmp_.set_network_out(ip4_top);

  // UDP4 -> IP4
  udp_.set_network_out(ip4_top);

  // TCP -> IP4
  tcp_.set_network_out(ip4_top);

  // IP4 -> Arp
  ip4_.set_linklayer_out(arp_top);

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

void Inet4::error_report(Error& err, Packet_ptr orig_pckt) {
  auto pckt_ip4 = static_unique_ptr_cast<PacketIP4>(std::move(orig_pckt));
  bool too_big = false;

  // Get the destination to the original packet
  Socket dest = [](const PacketIP4& pkt)->Socket {
    switch (pkt.ip_protocol()) {
      case Protocol::UDP: {
        const auto& udp = static_cast<const PacketUDP&>(pkt);
        return udp.destination();
      }
      case Protocol::TCP: {
        const auto& tcp = static_cast<const tcp::Packet&>(pkt);
        return tcp.destination();
      }
      default:
        return {};
    }
  }(*pckt_ip4);


  if (err.is_icmp()) {
    auto* icmp_err = dynamic_cast<ICMP_error*>(&err);

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

void Inet4::negotiate_dhcp(double timeout, dhcp_timeout_func handler) {
  INFO("Inet4", "Negotiating DHCP (%.1fs timeout)...", timeout);
  if (!dhcp_)
      dhcp_ = std::make_shared<DHClient>(*this);
  // @timeout for DHCP-server negotation
  dhcp_->negotiate(timeout);
  // add timeout_handler if supplied
  if (handler)
      dhcp_->on_config(handler);
}

void Inet4::network_config(IP4::addr addr,
                           IP4::addr nmask,
                           IP4::addr gateway,
                           IP4::addr dns)
{
  this->ip4_addr_   = addr;
  this->netmask_    = nmask;
  this->gateway_    = gateway;
  this->dns_server_ = (dns == IP4::ADDR_ANY) ? gateway : dns;
  INFO("Inet4", "Network configured (%s)", nic_.mac().to_string().c_str());
  INFO2("IP: \t\t%s", ip4_addr_.str().c_str());
  INFO2("Netmask: \t%s", netmask_.str().c_str());
  INFO2("Gateway: \t%s", gateway_.str().c_str());
  INFO2("DNS Server: \t%s", dns_server_.str().c_str());

  for(auto& handler : configured_handlers_)
    handler(*this);

  configured_handlers_.clear();
}

void Inet4::enable_conntrack(std::shared_ptr<Conntrack> ct)
{
  Expects(conntrack_ == nullptr && "Conntrack is already set");
  conntrack_ = ct;
}

void Inet4::process_sendq(size_t packets) {

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

void Inet4::force_start_send_queues()
{
  size_t packets = transmit_queue_available();
  if (packets) process_sendq(packets);
}

void Inet4::move_to_this_cpu()
{
  this->cpu_id = SMP::cpu_id();
  nic_.move_to_this_cpu();
}
