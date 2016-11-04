//-*- C++ -*-
#define DEBUG
#include <os>
#include <net/inet4.hpp>
#include <net/dhcp/dh4client.hpp>

using namespace net;

Inet4::Inet4(hw::Nic& nic)
  : ip4_addr_(IP4::ADDR_ANY),
    netmask_(IP4::ADDR_ANY),
    gateway_(IP4::ADDR_ANY),
    dns_server(IP4::ADDR_ANY),
    nic_(nic), arp_(*this), ip4_(*this),
    icmp_(*this), udp_(*this), tcp_(*this), dns(*this),
    bufstore_(nic.bufstore()), MTU_(nic.MTU())
{
  INFO("Inet4","Bringing up a IPv4 stack");
  Ensures(sizeof(IP4::addr) == 4);

  /** Upstream delegates */
  auto arp_bottom(upstream{arp_, &Arp::bottom});
  auto ip4_bottom(upstream{ip4_, &IP4::bottom});
  auto icmp4_bottom(upstream{icmp_, &ICMPv4::bottom});
  auto udp4_bottom(upstream{udp_, &UDP::bottom});
  auto tcp_bottom(upstream{tcp_, &TCP::bottom});

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
  auto arp_top(downstream{arp_, &Arp::transmit});
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
}

void Inet4::negotiate_dhcp(double timeout, dhcp_timeout_func handler) {
  INFO("Inet4", "Negotiating DHCP...");
  if(!dhcp_)
    dhcp_ = std::make_shared<DHClient>(*this);
  // @timeout for DHCP-server negotation
  dhcp_->negotiate(timeout);
  // add timeout_handler if supplied
  if(handler)
    dhcp_->on_config(handler);
}

void Inet4::on_config(dhcp_timeout_func handler) {
  // setup DHCP if not intialized
  if(!dhcp_)
    negotiate_dhcp();

  dhcp_->on_config(handler);
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
