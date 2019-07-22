
#include <hw/mac_addr.hpp>
#include <common.cxx>

using namespace std::string_literals;

extern lest::tests & specification();

CASE("MAC addresses can be compared")
{
  const MAC::Addr host_mac_address {0,240,34,255,45,11};
  const MAC::Addr host_mac_address_hex {0x00,0xf0,0x22,0xff,0x2d,0x0b};
  const MAC::Addr gateway_mac_address {0xb8,0xe8,0x56,0x4a,0x75,0x6e};
  EXPECT_NOT(host_mac_address == gateway_mac_address);
  EXPECT(host_mac_address == host_mac_address_hex);
}

CASE("MAC addresses can be assigned")
{
  MAC::Addr addr;
  const MAC::Addr host_mac_address {0,240,34,255,45,11};
  addr = host_mac_address;
  EXPECT(host_mac_address == addr);
}

CASE("MAC address string representation prints leading zeros")
{
  const MAC::Addr host_mac_address {0,240,34,255,45,11};
  auto mac_address_string = host_mac_address.str();
  EXPECT_NOT(mac_address_string == "0:f0:22:ff:2d:b");
  EXPECT(mac_address_string == "00:f0:22:ff:2d:0b");
}
