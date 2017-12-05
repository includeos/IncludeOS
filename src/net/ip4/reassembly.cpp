#include <net/ip4/ip4.hpp>

namespace net
{
  __attribute__((weak))
  IP4::IP_packet_ptr IP4::reassemble(IP4::IP_packet_ptr packet)
  {
    assert(packet != nullptr);
    return nullptr;
  }
}
