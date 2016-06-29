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

#include <stdexcept>
#include <vector>

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
    using buffer_t = uint8_t*;
    using release_del = delegate<void(buffer_t, size_t)>;

    BufferStore(size_t num, size_t bufsize);

    /** Free all the buffers **/
    ~BufferStore();

    /** Get a free buffer */
    buffer_t get_buffer();

    /** Return a buffer. */
    void release_buffer(buffer_t);

    /** Get size of a buffer **/
    inline size_t bufsize() const
    { return bufsize_; }

    inline size_t poolsize() const
    { return poolsize_; }

    /** @return the total buffer capacity in bytes */
    inline size_t capacity() const
    { return available_buffers_.size() * bufsize_; }

    /** Check if a buffer belongs here */
    inline bool address_is_from_pool(buffer_t addr)
    { return addr >= pool_ and addr < pool_ + (bufcount_ * bufsize_); }

    /** Check if an address is the start of a buffer */
    inline bool address_is_bufstart(buffer_t addr)
    { return (addr - pool_) % bufsize_ == 0; }

    inline size_t buffers_available()
    { return available_buffers_.size(); }

  private:
    size_t               poolsize_;
    const size_t         bufsize_;
    buffer_t             pool_;
    std::vector<buffer_t> available_buffers_;

    /** Delete move and copy operations **/
    BufferStore(BufferStore&)  = delete;
    BufferStore(BufferStore&&) = delete;
    BufferStore& operator=(BufferStore&)  = delete;
    BufferStore  operator=(BufferStore&&) = delete;

    /** Prohibit default construction **/
    BufferStore() = delete;
  }; //< class BufferStore
} //< namespace net

#endif //< NET_BUFFER_STORE_HPP
