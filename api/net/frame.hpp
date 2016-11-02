
#pragma once
#ifndef NET_FRAME_HPP
#define NET_FRAME_HPP

#include "packet.hpp"

namespace net {

  class Frame;
  using Frame_ptr = std::unique_ptr<Frame>;

  /** Anonymous link Frame, used for casting purposes */
  class Frame : public net::Packet {

  }; // < class Frame
}


#endif
