#pragma once

#include <stdint.h>
#include <net/ip6/ip6.hpp>

namespace net
{
  class NDP
  {
  public:
    struct router_sol
    {
      uint8_t  type;
      uint8_t  code;
      uint16_t checksum;
      uint32_t reserved;
      uint8_t  options[0];
      
    } __attribute__((packed));
    
    struct neighbor_sol
    {
      uint8_t   type;
      uint8_t   code;
      uint16_t  checksum;
      uint32_t  reserved;
      IP6::addr target;
      uint8_t   options[0];
      
    } __attribute__((packed));
    
    struct neighbor_adv
    {
      uint8_t   type;
      uint8_t   code;
      uint16_t  checksum;
      uint32_t  rso_reserved;
      IP6::addr target;
      uint8_t   options[0];
      
      static const uint32_t R = 0x1;
      static const uint32_t S = 0x2;
      static const uint32_t O = 0x4;
      
      void set_rso(uint32_t rso_flags)
      {
        rso_reserved = htonl(rso_flags & 7);
      }
      
    } __attribute__((packed));
    
    
    
  };
}
