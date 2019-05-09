#pragma once
#include <net/inet>
#include <hw/drivers/usernet.hpp>
#include <hw/async_device.hpp>

namespace fuzzy
{
  using AsyncDevice     = hw::Async_device<UserNet>;
  using AsyncDevice_ptr = std::unique_ptr<AsyncDevice>;

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
  insert_into_stack(AsyncDevice_ptr&, stack_config config,
                    const uint8_t* data, const size_t size);
}
