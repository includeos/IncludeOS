#pragma once
#include <net/inet>
#include "../router/async_device.hpp"

namespace fuzzy
{
  using AsyncDevice = std::unique_ptr<Async_device>;

  enum layer_t {
    ETH,
    IP4,
    TCP,
    TCP_CONNECTION,
    UDP,
    DNS,
    HTTP,
    WEBSOCKET
  };

  struct stack_config {
    layer_t  layer   = IP4;
    uint16_t ip_port = 0;
  };

  extern void
  insert_into_stack(AsyncDevice&, stack_config config,
                    const uint8_t* data, const size_t size);
}
