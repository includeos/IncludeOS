
#pragma once
#ifndef NET_LINK_LAYER_HPP
#define NET_LINK_LAYER_HPP

#include <hw/nic.hpp>

namespace net {

extern void set_last_packet(net::Packet*);

template <class T>
class Link_layer : public hw::Nic {
public:
  using Protocol    = T;
  using upstream    = hw::Nic::upstream;
  using downstream_link  = hw::Nic::downstream;
public:
  explicit Link_layer(Protocol&& protocol);

  std::string device_name() const override {
    return link_.link_name();
  }

  downstream_link create_link_downstream() override
  { return {link_, &Protocol::transmit}; }

  net::upstream_ip& ip4_upstream() override
  { return link_.ip4_upstream(); }

  net::upstream_ip& ip6_upstream() override
  { return link_.ip6_upstream(); }

  upstream& arp_upstream() override
  { return link_.arp_upstream(); }

  void set_ip4_upstream(upstream_ip handler) override
  { link_.set_ip4_upstream(handler); }

  void set_ip6_upstream(upstream_ip handler) override
  { link_.set_ip6_upstream(handler); }

  void set_arp_upstream(upstream handler) override
  { link_.set_arp_upstream(handler); }

  void set_vlan_upstream(upstream handler) override
  { link_.set_vlan_upstream(handler); }

  /** Number of bytes in a frame needed by the linklayer **/
  size_t frame_offset_link() const noexcept override
  { return Protocol::header_size(); }

  hw::Nic::Proto proto() const override
  { return Protocol::proto(); }

  /** Stats getters **/
  uint64_t get_packets_rx() override
  { return link_.get_packets_rx(); }

  uint64_t get_packets_tx() override
  { return link_.get_packets_tx(); }

  uint64_t get_packets_dropped() override
  { return link_.get_packets_dropped(); }


protected:
  /** Called by the underlying physical driver inheriting the Link_layer */
  void receive(net::Packet_ptr pkt)
  {
    set_last_packet(pkt.get());
    link_.receive(std::move(pkt));
  }

private:
  Protocol link_;
};

template <class Protocol>
Link_layer<Protocol>::Link_layer(Protocol&& protocol)
  : hw::Nic(),
    link_{std::forward<Protocol>(protocol)}
{
}

} // < namespace net

#endif
