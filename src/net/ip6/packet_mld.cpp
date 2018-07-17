//#define MLD_DEBUG 1
#ifdef MLD_DEBUG
#define PRINT(fmt, ...) printf(fmt, ##__VA_ARGS__)
#else
#define PRINT(fmt, ...) /* fmt */

#include <vector>
#include <net/ip6/packet_mld.hpp>
#include <net/ip6/packet_icmp6.hpp>
#include <statman>

namespace net::mld
{
  MldPacket2::MldPacket2(icmp6::Packet& icmp6) : icmp6_(icmp6) {}

  void mld_send_report(ip6::Addr mcast)
  {
    icmp6::Packet req(inet_.ip6_packet_factory());

    req.ip().set_ip_src(inet_.ip6_addr());

    req.ip().set_ip_hop_limit(1);
    req.set_type(ICMP_type::MULTICAST_LISTENER_REPORT);
    req.set_code(0);
    req.set_reserved(0);

    req.ip().set_ip_dst(ip6::Addr::node_all_routers);

    // Set target address
    req.add_payload(mcast.data(), IP6_ADDR_BYTES);
    req.set_checksum();

    PRINT("MLD: Sending MLD report: %i payload size: %i,"
        "checksum: 0x%x\n, source: %s, dest: %s\n",
        req.ip().size(), req.payload().size(), req.compute_checksum(),
        req.ip().ip_src().str().c_str(),
        req.ip().ip_dst().str().c_str());
  }
}
#endif
