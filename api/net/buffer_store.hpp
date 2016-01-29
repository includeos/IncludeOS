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

#ifndef NET_BUFFER_STORE_HPP
#define NET_BUFFER_STORE_HPP

#include <deque>
#include <stdexcept>

#include <net/inet_common.hpp>

namespace net{

  /**
   * Network buffer storage for uniformly sized buffers.
   *
   * @note : The buffer store is intended to be used by Packet, which is
   * a semi-intelligent buffer wrapper, used throughout the IP-stack.
   *
   * There shouldn't be any need for raw buffers in services.
   **/
  class BufferStore {
  public:
    using release_del = delegate<void(buffer, size_t)>;

    BufferStore(size_t num, size_t bufsize, size_t device_offset);

    /** Free all the buffers **/
    ~BufferStore();

    /** Get a free buffer */
    buffer get_raw_buffer();

    /** Get a free buffer, offset by device-offset */
    buffer get_offset_buffer();

    /** Return a buffer. */
    void release_raw_buffer(buffer b, size_t);

    /** Return a buffer, offset by offset_ bytes from actual buffer. */
    void release_offset_buffer(buffer b, size_t);

    /** Get size of a raw buffer **/
    inline size_t raw_bufsize()
    { return bufsize_; }

    inline size_t offset_bufsize()
    { return bufsize_ - device_offset_; }

    /** @return the total buffer capacity in bytes */
    inline size_t capacity()
    { return available_buffers_.size() * bufsize_; }

    /** Check if a buffer belongs here */
    inline bool address_is_from_pool(buffer addr)
    { return addr >= pool_ and addr < pool_ + (bufcount_ * bufsize_); }

    /** Check if an address is the start of a buffer */
    inline bool address_is_bufstart(buffer addr)
    { return (addr - pool_) % bufsize_ == 0; }

    /** Check if an address is the start of a buffer */
    inline bool address_is_offset_bufstart(buffer addr)
    { return (addr - pool_ - device_offset_) % bufsize_ == 0; }
  private:
    size_t             bufcount_;
    const size_t       bufsize_;
    size_t             device_offset_;
    buffer             pool_;
    std::deque<buffer> available_buffers_;

    /** Delete move and copy operations **/
    BufferStore(BufferStore&)  = delete;
    BufferStore(BufferStore&&) = delete;
    BufferStore& operator=(BufferStore&)  = delete;
    BufferStore  operator=(BufferStore&&) = delete;

    /** Prohibit default construction **/
    BufferStore() = delete;

    void increaseStorage();
  }; //< class BufferStore
} //< namespace net

#endif //< NET_BUFFER_STORE_HPP
