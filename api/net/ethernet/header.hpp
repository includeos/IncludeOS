
#pragma once
#ifndef NET_ETHERNET_HEADER_HPP
#define NET_ETHERNET_HEADER_HPP

#include <hw/mac_addr.hpp>
#include "ethertype.hpp"

namespace net {
namespace ethernet {

using trailer_t = uint32_t;

class Header {

  MAC::Addr dest_;
  MAC::Addr src_;
  Ethertype type_;

public:
  const MAC::Addr& dest() const noexcept
  { return dest_; }

  const MAC::Addr& src() const noexcept
  { return src_; }

  Ethertype type() const noexcept
  { return type_; }

  void set_dest(const MAC::Addr& dest)
  { dest_ = dest; }

  void set_src(const MAC::Addr& src)
  { src_ = src; }

  void set_type(Ethertype t)
  { type_ = t; }

} __attribute__((packed)) ;

}
}

#endif
