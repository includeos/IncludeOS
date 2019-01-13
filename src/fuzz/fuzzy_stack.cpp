#include <fuzz/fuzzy_stack.hpp>
#include <fuzz/fuzzy_packet.hpp>

static inline uint16_t udp_port_scan(net::Inet& inet)
{
  for (uint32_t udp_port = 1; udp_port < 65536; udp_port++) {
    if (inet.udp().is_bound({inet.ip_addr(), (uint16_t) udp_port})) {
      return udp_port;
    }
  }
  return 0;
}

namespace fuzzy
{
  void insert_into_stack(AsyncDevice_ptr& device, stack_config config,
                         const uint8_t* data, const size_t size)
  {
    auto& inet = net::Interfaces::get(0);
    const size_t packet_size = std::min((size_t) inet.MTU(), size);
    FuzzyIterator fuzzer{data, packet_size};
    
    auto p = inet.create_packet();
    // link layer -> IP4
    auto* eth_end = add_eth_layer(p->layer_begin(), fuzzer,
                                  net::Ethertype::IP4);
    // select layer to fuzz
    switch (config.layer) {
    case ETH:
      // by subtracting 2 i can fuzz ethertype as well
      fuzzer.fill_remaining(eth_end);
      break;
    case IP4:
      {
        auto* next_layer = add_ip4_layer(eth_end, fuzzer,
                           {10, 0, 0, 1}, inet.ip_addr());
        fuzzer.fill_remaining(next_layer);
        break;
      }
    case UDP:
      {
        // scan for UDP port (once)
        static uint16_t udp_port = 0;
        if (udp_port == 0) {
          udp_port = udp_port_scan(inet);
          assert(udp_port != 0);
        }
        // generate IP4 and UDP datagrams
        auto* ip_layer = add_ip4_layer(eth_end, fuzzer,
                           {10, 0, 0, 1}, inet.ip_addr(),
                            (uint8_t) net::Protocol::UDP);
        auto* udp_layer = add_udp4_layer(ip_layer, fuzzer,
                            udp_port);
        fuzzer.fill_remaining(udp_layer);
        break;
      }
    case TCP:
      {
        assert(config.ip_port != 0 && "Port must be set in the config");
        // generate IP4 and TCP data
        auto* ip_layer = add_ip4_layer(eth_end, fuzzer,
                           {10, 0, 0, 1}, inet.ip_addr(),
                            (uint8_t) net::Protocol::TCP);
        auto* tcp_layer = add_tcp4_layer(ip_layer, fuzzer,
                            config.ip_port);
        fuzzer.fill_remaining(tcp_layer);
        break;
      }
    case TCP_CONNECTION:
      //
      break;
    default:
      assert(0 && "Implement me");
    }
    // we have to add ethernet size here as its not part of MTU
    p->set_data_end(fuzzer.data_counter);
    device->nic().receive(std::move(p));
  }
}
