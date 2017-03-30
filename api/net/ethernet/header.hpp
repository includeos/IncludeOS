// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015-2016 Oslo and Akershus University College of Applied Sciences
// and Alfred Bratterud
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

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
