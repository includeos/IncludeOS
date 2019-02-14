#pragma once
#include <net/inet>
#include <hw/async_device.hpp>
#include <hw/usernet.hpp>

namespace fuzzy
{
  using AsyncDevice     = hw::Async_device<UserNet>;
  using AsyncDevice_ptr = std::unique_ptr<AsyncDevice>;

  enum layer_t {
    ETH,
    IP4,
    TCP,
    UDP,
    IP6,
    ICMP6,
    UDP6,
    TCP6,
    DNS,
    HTTP,
    WEBSOCKET
  };

  struct stack_config {
    layer_t  layer   = IP4;
    uint16_t ip_port = 0;
    uint16_t ip_src_port = 0;
    uint32_t tcp_seq;
    uint32_t tcp_ack;
  };

  extern void
  insert_into_stack(AsyncDevice_ptr&, stack_config config,
                    const uint8_t* data, const size_t size);
}
