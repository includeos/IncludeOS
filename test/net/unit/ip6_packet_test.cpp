
#include <packet_factory.hpp>
#include <common.cxx>
#include <net/ip6/addr.hpp>

using namespace net;

CASE("IP6 Packet HOPLIMIT")
{
  const ip6::Addr src{0xfe80, 0, 0, 0, 0xe823, 0xfcff, 0xfef4, 0x85bd};
  const ip6::Addr dst{ 0xfe80,  0,  0, 0, 0xe823, 0xfcff, 0xfef4, 0x83e7 };
  auto ip6 = create_ip6_packet_init(src, dst);

  const uint8_t DEFAULT_HOPLIMIT = 128;

  ip6->set_ip_hop_limit(DEFAULT_HOPLIMIT);

  EXPECT(ip6->hop_limit() == DEFAULT_HOPLIMIT);

  for(uint8_t i = DEFAULT_HOPLIMIT; i > 0; i--) {
    ip6->decrement_hop_limit();
    EXPECT(ip6->hop_limit() == i-1);
  }
}

CASE("IP6 Packet HOPLIMIT - multiple packets")
{
  std::vector<ip6::Addr> addrs{
    {0xfe80, 0, 0, 0, 0xe823, 0xfcff, 0xfef4, 0x85bd},
    {0xfe80,  0,  0, 0, 0xe823, 0xfcff, 0xfef4, 0x83e7},
    ip6::Addr{0,0,0,1},
    {0xfe80, 0, 0, 0, 0x0202, 0xb3ff, 0xff1e, 0x8329},
  };

  for(int i = 0; i < addrs.size()-1; i++)
  {
    auto ip6 = create_ip6_packet_init(addrs[i], addrs[i+1]);

    const uint8_t DEFAULT_HOPLIMIT = 64;

    ip6->set_ip_hop_limit(DEFAULT_HOPLIMIT);

    EXPECT(ip6->hop_limit() == DEFAULT_HOPLIMIT);

    for(uint8_t i = DEFAULT_HOPLIMIT; i > 0; i--)
      ip6->decrement_hop_limit();

    EXPECT(ip6->hop_limit() == 0);
  }
}
