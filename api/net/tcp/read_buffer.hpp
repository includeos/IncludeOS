// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015-2017 Oslo and Akershus University College of Applied Sciences
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
#ifndef NET_TCP_READ_BUFFER_HPP
#define NET_TCP_READ_BUFFER_HPP

#include "common.hpp" // buffer_t

namespace net {
namespace tcp {

/**
 * @brief      A TCP read buffer, used for adding incoming data
 *             into a specific place in a buffer based on sequence number.
 *
 *             Supports hole in the sequence space, but do NOT support
 *             duplicate/overlapping sequences (this is the responsible
 *             of the TCP implementation)
 */
class Read_buffer {
public:
  using Alloc = os::mem::buffer::allocator_type;
  /**
   * @brief      Construct a read buffer.
   *             Min and max need to be power of 2.
   *
   * @param[in]  start  The sequence number the buffer starts on
   * @param[in]  min    The minimum size of the buffer (preallocated)
   * @param[in]  max    The maximum size of the buffer (how much it can grow)
   */
  Read_buffer(const seq_t start, const size_t min, const size_t max);

  Read_buffer(const seq_t start, const size_t min, const size_t max, const Alloc& alloc);

  /**
   * @brief      Insert data into the buffer relative to the sequence number.
   *
   * @warning    Duplicate/overlapping sequences WILL break the buffer.
   *
   * @param[in]  seq   The sequence number the data starts on
   * @param[in]  data  The data
   * @param[in]  len   The length of the data
   * @param[in]  push  Whether push or not
   *
   * @return     Bytes inserted
   */
  size_t insert(const seq_t seq, const uint8_t* data, size_t len, bool push = false);

  /**
   * @brief      Returns the amount of the bytes that fits in the buffer
   *             when starting from "seq".
   *             If the seq is before the start of the buffer, 0 is returned.
   *
   * @param[in]  seq   The sequence number
   *
   * @return     Amount of bytes that fits in the buffer starting from seq
   */
  size_t fits(const seq_t seq) const
  {
    const auto rel = (seq - start);
    return (rel < capacity()) ? (capacity() - rel) : 0;
  }

  /**
   * @brief      Exposes the internal buffer
   *
   * @return     A reference to the internal shared buffer
   */
  buffer_t buffer()
  { return buf; }

  /**
   * @brief  Check if internal buffer has unhandled data
   *
   * @return True if the internal buffer is unique with data and doesnt contain hole
  */
  bool has_unhandled_data()
  { return (buf.unique() && (size() > 0) && (missing() == 0)); }

  /**
   * @brief      Exposes the internal buffer (read only)
   *
   * @return     A const reference to the internal shared buffer
   */
  const buffer_t& buffer() const
  { return buf; }

  /**
   * @brief      The capacity of the internal buffer
   *
   * @return     The capacity
   */
  size_t capacity() const noexcept
  { return cap; }

  /**
   * @brief      How far into the internal buffer data has been written.
   *             (Do not account for holes)
   *
   * @return     Where data ends
   */
  size_t size() const noexcept
  { return buf->size(); }

  /**
   * @brief      The amount of bytes missing in the internal buffer (holes).
   *
   * @return     Bytes missing as holes in the buffer
   */
  size_t missing() const noexcept
  { return hole; }

  /**
   * @brief      Whether size has reached the end of the buffer
   *
   * @return     Whether size has reached the end of the buffer
   */
  bool at_end() const noexcept
  { return size() == capacity(); }

  /**
   * @brief      Determines if the buffer is ready "for delivery".
   *             When there is no holes, and either PUSH has been seen
   *             or the buffer is full.
   *
   * @return     True if ready, False otherwise.
   */
  bool is_ready() const noexcept
  { return (push_seen or at_end()) and hole == 0; }

  /**
   * @brief      Reset this buffer, initialize it for a new sequence start.
   *             Creates a new internal buffer ONLY IF needed.
   *
   * @param[in]  seq   The new starting sequence number
   */
  void reset(const seq_t start);

  /**
   * @brief      Reset the buffer as reset(seq_t), but with a given capacity.
   *
   * @param[in]  start     The start
   * @param[in]  capacity  The capacity
   */
  void reset(const seq_t start, const size_t capacity);

  /**
   * @brief      Sets the starting sequence number.
   *             Should not be messed with.
   *
   * @param[in]  seq   The sequence number
   */
  void set_start(const seq_t seq)
  { start = seq; }

  seq_t start_seq() const
  { return start; }

  seq_t end_seq() const
  { return start + capacity(); }


  int deserialize_from(void*);
  int serialize_to(void*) const;

private:
  buffer_t        buf;
  seq_t           start;
  size_t          cap;
  int32_t         hole; // number of bytes missing
  bool            push_seen{false};

  /**
   * @brief      Reset the buffer if non-unique
   */
  void reset_buffer_if_needed();

}; // < class Read_buffer

} // < namespace tcp
} // < namespace net

#endif // < NET_TCP_READ_BUFFER_HPP
