
#pragma once
#ifndef NET_ETHERNET_8021Q_HPP
#define NET_ETHERNET_8021Q_HPP

#include "ethernet.hpp"

namespace net {

namespace ethernet {
/**
 * @brief      IEEE 802.1Q header
 *
 *             Currently no support for Double tagging.
 */
struct VLAN_header {
  MAC::Addr dest;
  MAC::Addr src;
  uint16_t  tpid;
  uint16_t  tci;
  Ethertype type;

  int vid() const
  { return ntohs(this->tci) & 0xFFF; }

  void set_vid(int id)
  { tci = htons(id & 0xFFF); }

}__attribute__((packed));

} // < namespace ethernet

/**
 * @brief      Ethernet module with support for VLAN.
 *
 * @note       Inheritance out of lazyness.
 */
class Ethernet_8021Q : public Ethernet {
public:
  using header  = ethernet::VLAN_header;

  /**
   * @brief      Constructs a Ethernet_8021Q object,
   *             same as Ethernet with addition to the VLAN id
   *
   * @param[in]  phys_down  The physical down
   * @param[in]  mac        The mac addr
   * @param[in]  id         The VLAN id
   */
  explicit Ethernet_8021Q(downstream phys_down, const addr& mac, const int id) noexcept;

  void receive(Packet_ptr pkt);

  void transmit(Packet_ptr pkt, addr dest, Ethertype type);

  static constexpr uint16_t header_size() noexcept
  { return sizeof(ethernet::VLAN_header); }

private:
  const int id_;
};

} // < namespace net

#endif
