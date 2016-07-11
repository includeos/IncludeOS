// License

#pragma once
#ifndef NET_TCP_OPTION_HPP
#define NET_TCP_OPTION_HPP

#include <net/util.hpp> // byte ordering

namespace net {
namespace tcp {

/*
  TCP Header Option
*/
struct Option {
  uint8_t kind;
  uint8_t length;
  uint8_t data[0];

  enum Kind {
    END = 0x00, // End of option list
    NOP = 0x01, // No-Opeartion
    MSS = 0x02, // Maximum Segment Size [RFC 793] Rev: [879, 6691]
  };

  static std::string kind_string(Kind kind) {
    switch(kind) {
    case MSS:
      return {"MSS"};

    default:
      return {"Unknown Option"};
    }
  }

  struct opt_mss {
    uint8_t kind;
    uint8_t length;
    uint16_t mss;

    opt_mss(uint16_t mss)
      : kind(MSS), length(4), mss(htons(mss)) {}
  };

  struct opt_timestamp {
    uint8_t kind;
    uint8_t length;
    uint32_t ts_val;
    uint32_t ts_ecr;
  };

}; // < struct Option

} // < namespace net
} // < namespace tcp

#endif // < NET_TCP_OPTION_HPP
