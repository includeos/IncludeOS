
#pragma once
#ifndef NET_ETHERNET_FRAME_HPP
#define NET_ETHERNET_FRAME_HPP

#include "../frame.hpp"

namespace net {
  namespace ethernet {

    class Frame;
    using Frame_ptr = std::unique_ptr<Frame>;

    class Frame : public net::Frame {

    };

  } // < namespace ethernet
} // < namespace net

#endif
