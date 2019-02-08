
#include <net/addr.hpp>

// Initialization of static addresses.
namespace net {

  const Addr Addr::addr_any{};

  const ip4::Addr ip4::Addr::addr_any{0};
  const ip4::Addr ip4::Addr::addr_bcast{0xff,0xff,0xff,0xff};

  const ip6::Addr ip6::Addr::node_all_nodes(0xFF01, 0, 0, 0, 0, 0, 0, 1);
  const ip6::Addr ip6::Addr::node_all_routers(0xFF01, 0, 0, 0, 0, 0, 0, 2);
  const ip6::Addr ip6::Addr::node_mDNSv6(0xFF01, 0, 0, 0, 0, 0, 0, 0xFB);

  const ip6::Addr ip6::Addr::link_unspecified(0, 0, 0, 0, 0, 0, 0, 0);
  const ip6::Addr ip6::Addr::addr_any{ip6::Addr::link_unspecified};

  const ip6::Addr ip6::Addr::link_all_nodes(0xFF02, 0, 0, 0, 0, 0, 0, 1);
  const ip6::Addr ip6::Addr::link_all_routers(0xFF02, 0, 0, 0, 0, 0, 0, 2);
  const ip6::Addr ip6::Addr::link_mDNSv6(0xFF02, 0, 0, 0, 0, 0, 0, 0xFB);

  const ip6::Addr ip6::Addr::link_dhcp_servers(0xFF02, 0, 0, 0, 0, 0, 0x01, 0x02);
  const ip6::Addr ip6::Addr::site_dhcp_servers(0xFF05, 0, 0, 0, 0, 0, 0x01, 0x03);
}
