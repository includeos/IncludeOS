#include <fuzz/fuzzy_packet.hpp>
#include <net/ethernet/header.hpp>
#define htons(x) __builtin_bswap16(x)

namespace fuzzy
{
  uint8_t*
  add_eth_layer(uint8_t* data, FuzzyIterator& fuzzer, net::Ethertype type)
  {
    auto* eth = (net::ethernet::Header*) data;
    eth->set_src({0x1, 0x2, 0x3, 0x4, 0x5, 0x6});
    eth->set_dest({0x7, 0x8, 0x9, 0xA, 0xB, 0xC});
    eth->set_type(type);
    fuzzer.increment_data(sizeof(net::ethernet::Header));
    return &data[sizeof(net::ethernet::Header)];
  }
  uint8_t*
  add_ip4_layer(uint8_t* data, FuzzyIterator& fuzzer,
                const net::ip4::Addr src_addr,
                const net::ip4::Addr dst_addr,
                const uint8_t protocol)
  {
    auto* hdr = new (data) net::ip4::Header();
    hdr->ttl      = 64;
    hdr->protocol = (protocol) ? protocol : fuzzer.steal8();
    hdr->check    = 0;
    hdr->tot_len  = htons(sizeof(net::ip4::Header) + fuzzer.size);
    hdr->saddr    = src_addr;
    hdr->daddr    = dst_addr;
    //hdr->check    = net::checksum(hdr, sizeof(net::ip4::header));
    fuzzer.increment_data(sizeof(net::ip4::Header));
    return &data[sizeof(net::ip4::Header)];
  }
  uint8_t*
  add_udp4_layer(uint8_t* data, FuzzyIterator& fuzzer,
                const uint16_t dport)
  {
    auto* hdr = new (data) net::udp::Header();
    hdr->sport = htons(fuzzer.steal16());
    hdr->dport = htons(dport);
    hdr->length = htons(fuzzer.size);
    hdr->checksum = 0;
    fuzzer.increment_data(sizeof(net::udp::Header));
    return &data[sizeof(net::udp::Header)];
  }
  uint8_t*
  add_tcp4_layer(uint8_t* data, FuzzyIterator& fuzzer,
                const uint16_t dport)
  {
    auto* hdr = new (data) net::tcp::Header();
    hdr->source_port      = htons(1234);
    hdr->destination_port = htons(dport);
    hdr->seq_nr      = fuzzer.steal32();
    hdr->ack_nr      = fuzzer.steal32();
    hdr->offset_flags.offset_reserved = 0;
    hdr->offset_flags.flags = fuzzer.steal8();
    hdr->window_size = fuzzer.steal16();
    hdr->checksum    = 0;
    hdr->urgent      = 0;
    fuzzer.increment_data(sizeof(net::tcp::Header));
    return &data[sizeof(net::tcp::Header)];
  }
}
