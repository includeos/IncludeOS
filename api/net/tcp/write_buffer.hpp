// License

#pragma once
#ifndef NET_TCP_WRITE_BUFFER_HPP
#define NET_TCP_WRITE_BUFFER_HPP

#include "common.hpp"

namespace net {
namespace tcp {
/*
  Wrapper around a buffer that contains data to be written.
*/
struct WriteBuffer {
  buffer_t buffer;
  size_t remaining;
  size_t offset;
  size_t acknowledged;
  bool push;

  WriteBuffer(buffer_t buf, size_t length, bool PSH, size_t offs = 0)
    : buffer(buf), remaining(length-offs), offset(offs), acknowledged(0), push(PSH) {}

  inline size_t length() const { return remaining + offset; }

  inline bool done() const { return acknowledged == length(); }

  inline uint8_t* begin() const { return buffer.get(); }

  inline uint8_t* pos() const { return buffer.get() + offset; }

  inline uint8_t* end() const { return buffer.get() + length(); }

  inline bool advance(size_t length) {
    assert(length <= remaining);
    offset += length;
    remaining -= length;
    return length > 0;
  }

  size_t acknowledge(size_t bytes) {
    auto acked = std::min(bytes, length()-acknowledged);
    acknowledged += acked;
    return acked;
  }

}; // < struct WriteBuffer

} // < namespace net
} // < namespace tcp

#endif // < NET_TCP_WRITE_BUFFER_HPP

