
#include <hw/nic.hpp>

namespace hw
{
  __attribute__((weak))
  uint16_t Nic::MTU_detection_override(int idx, const uint16_t default_MTU)
  {
    (void) idx;
    return default_MTU;
  }
}
