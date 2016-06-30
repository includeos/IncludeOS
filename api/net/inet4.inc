//-*- C++ -*-
#define DEBUG
#include <os>
#include "dhcp/dh4client.hpp"

namespace net
{
  template <typename T> inline
  Inet4<T>::Inet4(
    hw::Nic<T>& nic,
    IP4::addr ip,
    IP4::addr netmask,
    IP4::addr gateway)
    : ip4_addr_(ip), netmask_(netmask), router_(gateway),
      nic_(nic), eth_(nic.mac()), arp_(*this), ip4_(*this),
      icmp_(*this), udp_(*this), tcp_(*this), dns(*this),
      bufstore_(nic.bufstore())
  {
    INFO("Inet4","Bringing up the IP stack");
    Ensures(sizeof(IP4::addr) == 4);

    // default DNS server to gateway
    set_dns_server(gateway);

    /** Upstream delegates */
    auto eth_bottom(upstream::from<Ethernet,&Ethernet::bottom>(eth_));
    auto arp_bottom(upstream::from<Arp,&Arp::bottom>(arp_));
    auto ip4_bottom(upstream::from<IP4,&IP4::bottom>(ip4_));
    auto icmp4_bottom(upstream::from<ICMPv4,&ICMPv4::bottom>(icmp_));
    auto udp4_bottom(upstream::from<UDP,&UDP::bottom>(udp_));
    auto tcp_bottom(upstream::from<TCP,&TCP::bottom>(tcp_));

    /** Upstream wiring  */
    // Packets available
    nic.on_transmit_queue_available(
      transmit_avail_delg::from<Inet4<T>, &Inet4<T>::process_sendq>(*this));

    // Phys -> Eth (Later, this will be passed through router)
    nic.set_linklayer_out(eth_bottom);

    // Eth -> Arp
    eth_.set_arp_handler(arp_bottom);

    // Eth -> IP4
    eth_.set_ip4_handler(ip4_bottom);

    // IP4 -> ICMP
    ip4_.set_icmp_handler(icmp4_bottom);

    // IP4 -> UDP
    ip4_.set_udp_handler(udp4_bottom);

    // IP4 -> TCP
    ip4_.set_tcp_handler(tcp_bottom);

    /** Downstream delegates */
    auto phys_top(downstream
                  ::from<hw::Nic<VirtioNet>,&hw::Nic<VirtioNet>::transmit>(nic));
    auto eth_top(downstream
                 ::from<Ethernet,&Ethernet::transmit>(eth_));
    auto arp_top(downstream
                 ::from<Arp,&Arp::transmit>(arp_));
    auto ip4_top(downstream
                 ::from<IP4,&IP4::transmit>(ip4_));

    /** Downstream wiring. */

    // ICMP -> IP4
    icmp_.set_network_out(ip4_top);

    // UDP4 -> IP4
    udp_.set_network_out(ip4_top);

    // TCP -> IP4
    tcp_.set_network_out(ip4_top);

    // IP4 -> Arp
    ip4_.set_linklayer_out(arp_top);

    // Arp -> Eth
    arp_.set_linklayer_out(eth_top);

    // Eth -> Phys
    eth_.set_physical_out(phys_top);
  }

  template <typename T> inline
  Inet4<T>::Inet4(hw::Nic<T>& nic, double timeout)
    : Inet4(nic, IP4::INADDR_ANY, IP4::INADDR_ANY, IP4::INADDR_ANY)
  {
    INFO("Inet4", "Trying DHCP...");
    dhcp_ = std::make_shared<DHClient>(*this);
    // @timeout for DHCP-server negotation
    dhcp_->negotiate(timeout);
  }

  template <typename T> inline
  void Inet4<T>::on_config(delegate<void(bool)> handler) {
    dhcp_->on_config(handler);
  }

  template <typename T> inline
  void Inet4<T>::process_sendq(size_t packets) {

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

}
