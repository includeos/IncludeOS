// License

#pragma once
#ifndef NET_TCP_READ_BUFFER_HPP
#define NET_TCP_READ_BUFFER_HPP

#include <cassert>

#include "common.hpp" // buffer_t

namespace net {
namespace tcp {
/*
  Wrapper around a buffer that receives data.
*/
struct ReadBuffer {
  buffer_t buffer;
  size_t remaining;
  size_t offset;
  bool push;

  ReadBuffer(buffer_t buf, size_t length, size_t offs = 0)
    : buffer(buf), remaining(length-offs), offset(offs), push(false) {}

  inline size_t capacity() const
  { return remaining + offset; }

  inline bool empty() const
  { return offset == 0; }

  inline bool full() const
  { return remaining == 0; }

  inline size_t size() const
  { return offset; }

  inline uint8_t* begin() const
  { return buffer.get(); }

  inline uint8_t* pos() const
  { return buffer.get() + offset; }

  inline uint8_t* end() const
  { return buffer.get() + capacity(); }

  bool advance(size_t length) {
    assert(length <= remaining);
    offset += length;
    remaining -= length;
    return length > 0;
  }

  void clear() {
    memset(begin(), 0, offset);
    remaining = capacity();
    offset = 0;
    push = false;
  }

  /*
    Renews the ReadBuffer by assigning a new buffer_t, releasing ownership
  */
  void renew() {
    remaining = capacity();
    offset = 0;
    buffer = buffer_t(new uint8_t[remaining], std::default_delete<uint8_t[]>());
    push = false;
  }
}; // < ReadBuffer

} // < namespace tcp
} // < namespace net

#endif // < NET_TCP_READ_BUFFER_HPP
