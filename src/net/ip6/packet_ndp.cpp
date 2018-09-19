#define NDP_DEBUG 1
#ifdef NDP_DEBUG
#define PRINT(fmt, ...) printf(fmt, ##__VA_ARGS__)
#else
#define PRINT(fmt, ...) /* fmt */
#endif

#include <vector>
#include <net/ip6/packet_ndp.hpp>
#include <net/ip6/packet_icmp6.hpp>
#include <statman>

namespace net::ndp {

  NdpPacket::NdpPacket(icmp6::Packet& icmp6) : icmp6_(icmp6), ndp_opt_() {}

  void NdpPacket::parse_options(icmp6::Type type)
  {
    switch(type) {
    case (ICMP_type::ND_ROUTER_SOL):
      ndp_opt_.parse(router_sol().options,
              (icmp6_.payload_len() - router_sol().option_offset()));
      break;
    case (ICMP_type::ND_ROUTER_ADV):
      ndp_opt_.parse(router_adv().options,
              (icmp6_.payload_len() - router_adv().option_offset()));
      break;
    case (ICMP_type::ND_NEIGHBOUR_SOL):
      ndp_opt_.parse(neighbour_sol().options,
              (icmp6_.payload_len() - neighbour_sol().option_offset()));
      break;
    case (ICMP_type::ND_NEIGHBOUR_ADV):
      ndp_opt_.parse(neighbour_adv().options,
              (icmp6_.payload_len() - neighbour_adv().option_offset()));
      break;
    case (ICMP_type::ND_REDIRECT):
      ndp_opt_.parse(router_redirect().options,
              (icmp6_.payload_len() - router_redirect().option_offset()));
      break;
    default:
      break;
    }
  }

  void NdpOptions::parse(uint8_t *opt, uint16_t opts_len)
  {
    uint16_t opt_len;
    header_ = reinterpret_cast<struct nd_options_header*>(opt);
    struct nd_options_header *option_hdr = header_;

    if (option_hdr == NULL) {
       return;
    }
    while(opts_len) {
      if (opts_len < sizeof (struct nd_options_header)) {
         return;
      }
      opt_len = option_hdr->len << 3;

      if (opts_len < opt_len || opt_len == 0) {
         return;
      }
      switch (option_hdr->type) {
      case ND_OPT_SOURCE_LL_ADDR:
      case ND_OPT_TARGET_LL_ADDR:
      case ND_OPT_MTU:
      case ND_OPT_NONCE:
      case ND_OPT_REDIRECT_HDR:
        if (opt_array[option_hdr->type]) {
        } else {
           opt_array[option_hdr->type] = option_hdr;
        }
        option_hdr = opt_array[option_hdr->type];
        break;
      case ND_OPT_PREFIX_INFO:
        opt_array[ND_OPT_PREFIX_INFO_END] = option_hdr;
        if (!opt_array[ND_OPT_PREFIX_INFO]) {
           opt_array[ND_OPT_PREFIX_INFO] = option_hdr;
        }
        break;
      case ND_OPT_ROUTE_INFO:
         nd_opts_ri_end = option_hdr;
         if (!nd_opts_ri) {
             nd_opts_ri = option_hdr;
         }
         break;
      default:
        if (is_useropt(option_hdr)) {
          user_opts_end = option_hdr;
          if (!user_opts) {
           user_opts = option_hdr;
          }
        } else {
          PRINT("%s: Unsupported option: type=%d, len=%d\n",
              __FUNCTION__, option_hdr->type, option_hdr->len);
        }
      }
      opts_len -= opt_len;
      option_hdr = (option_hdr + opt_len);
    }
 }

 bool NdpPacket::parse_prefix(Pinfo_handler autoconf_cb,
      Pinfo_handler onlink_cb)
 {
   return ndp_opt_.parse_prefix(autoconf_cb, onlink_cb);
 }

 bool NdpOptions::parse_prefix(Pinfo_handler autoconf_cb,
    Pinfo_handler onlink_cb)
 {
   ip6::Addr confaddr;
   struct prefix_info *pinfo;
   PRINT("about to parse opt\n");
   struct nd_options_header *opt = option(ND_OPT_PREFIX_INFO);
   PRINT("opt parsed\n");

   if (!opt) {
     return true;
   }

   for (pinfo = reinterpret_cast<struct prefix_info *>(opt); pinfo != nullptr;
        pinfo = pinfo_next(pinfo))
   {
    PRINT("pinfo start type=%u\n", pinfo->type);
    if(pinfo == nullptr)
      PRINT("pinfo null");

    PRINT("prefix %s\n", pinfo->prefix.to_string().c_str());
    if (pinfo->prefix.is_linklocal()) {
      PRINT("NDP: Prefix info address is linklocal\n");
      return false;
    }

    if (pinfo->onlink) {
      PRINT("on link\n");
      onlink_cb(confaddr, pinfo->prefered, pinfo->valid);
    }
    else if (pinfo->autoconf)
    {
      PRINT("autoconf\n");
       if (pinfo->prefix.is_multicast()) {
         PRINT("NDP: Prefix info address is multicast\n");
         return false;
       }

       if (pinfo->prefered > pinfo->valid) {
         PRINT("NDP: Prefix option has invalid lifetime\n");
         return false;
       }

       if (pinfo->prefix_len == 64) {
         confaddr.set_part<uint64_t>(1,
            pinfo->prefix.get_part<uint64_t>(1));
       }
       else {
         PRINT("NDP: Prefix option: autoconf: "
                 " prefix with wrong len: %d", pinfo->prefix_len);
         return false;
       }
       autoconf_cb(confaddr, pinfo->prefered, pinfo->valid);
     }
     PRINT("next\n");
   }
 }

  NdpPacket::RouterSol& NdpPacket::router_sol()
  { return *reinterpret_cast<RouterSol*>(&(icmp6_.header().payload[0])); }

  NdpPacket::RouterAdv& NdpPacket::router_adv()
  { return *reinterpret_cast<RouterAdv*>(&(icmp6_.header().payload[0])); }

  NdpPacket::RouterRedirect& NdpPacket::router_redirect()
  { return *reinterpret_cast<RouterRedirect*>(&(icmp6_.header().payload[0])); }

  NdpPacket::NeighborSol& NdpPacket::neighbour_sol()
  { return *reinterpret_cast<NeighborSol*>(&(icmp6_.header().payload[0])); }

  NdpPacket::NeighborAdv& NdpPacket::neighbour_adv()
  { return *reinterpret_cast<NeighborAdv*>(&(icmp6_.header().payload[0])); }

  bool NdpPacket::is_flag_router()
  { return icmp6_.header().rso_flags & NEIGH_ADV_ROUTER; }

  bool NdpPacket::is_flag_solicited()
  { return icmp6_.header().rso_flags & NEIGH_ADV_SOL; }

  bool NdpPacket::is_flag_override()
  { return icmp6_.header().rso_flags &  NEIGH_ADV_OVERRIDE; }

  void NdpPacket::set_neighbour_adv_flag(uint32_t flag)
  { icmp6_.header().rso_flags = htonl(flag << 28); }

  void NdpPacket::set_ndp_options_header(uint8_t type, uint8_t len)
  {
    struct nd_options_header header;
    header.type = type;
    header.len = len;

    icmp6_.add_payload(reinterpret_cast<uint8_t*>(&header),
            sizeof header);
  }
}
