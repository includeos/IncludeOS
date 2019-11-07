
#include <map>
#include <common.cxx>
#include <nic_mock.hpp>
#include <net/inet>

#define MYINFO(X,...) INFO("Unit IP6", X, ##__VA_ARGS__)

using namespace net;
CASE("IP6 packet setters/getters")
{
  INFO("Unit", "IP6 packet setters/getters");
  SETUP("A pre-wired IP6 instance"){
    Nic_mock nic;
    Inet inet{nic};

      SECTION("Inet-created IP packets are initialized correctly") {
        auto ip_pckt1 = inet.create_ip6_packet(Protocol::ICMPv4);
        EXPECT(ip_pckt1->ip6_version() == 6);
        EXPECT(ip_pckt1->is_ipv6());
        EXPECT(ip_pckt1->ip_dscp() == DSCP::CS0);
        EXPECT(ip_pckt1->ip_ecn() == ECN::NOT_ECT);
        EXPECT(ip_pckt1->flow_label() == 0);
        EXPECT(ip_pckt1->payload_length() == 0);
        EXPECT(ip_pckt1->next_protocol() == Protocol::ICMPv4);
        EXPECT(ip_pckt1->hop_limit() == PacketIP6::DEFAULT_HOP_LIMIT);
        EXPECT(ip_pckt1->ip_src() == IP6::ADDR_ANY);
        EXPECT(ip_pckt1->ip_dst() == IP6::ADDR_ANY);
      }
  }
}
