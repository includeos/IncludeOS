#pragma once

#include <stdint.h>

namespace net
{
  class NDP
  {
  public:
    struct solicitation_header
    {
      uint8_t  type;
      uint8_t  code;
      uint16_t checksum;
      uint32_t reserved;
      uint8_t  options[0];
      
    } __attribute__((packed));
    
    
    
  };
}
