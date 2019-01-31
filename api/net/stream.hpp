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
#ifndef NET_STREAM_HPP
#define NET_STREAM_HPP

#include <cstdint>
#include <cstddef>
#include <delegate>
#include <memory>
#include <pmr>
#include <vector>
#include <net/socket.hpp>

namespace net {
  class Stream;
  using Stream_ptr = std::unique_ptr<Stream>;
  /**
   * @brief      An abstract network Stream interface based on TCP.
   */
  class Stream {
  public:
    using buffer_t = os::mem::buf_ptr;
    using ptr      = Stream_ptr;

    /** Construct a shared vector used by streams **/
    template <typename... Args>
    static buffer_t construct_buffer(Args&&... args) {
      return std::make_shared<os::mem::buffer> (std::forward<Args> (args)...);
    }

    /** Called when the stream is ready to be used. */
    using ConnectCallback = delegate<void(Stream& self)>;
    /**
     * @brief      Event when the stream is connected/established/ready to be used.
     *
     * @param[in]  cb    The connect callback
     */
    virtual void on_connect(ConnectCallback cb) = 0;

    /** Called with a shared buffer and the length of the data when received. */
    using ReadCallback = delegate<void(buffer_t)>;
    /**
     * @brief      Event when data is received. Pushes data to the callback.
     *
     * @param[in]  n     The size of the receive buffer
     * @param[in]  cb    The read callback
     */
    virtual void on_read(size_t n, ReadCallback cb) = 0;

    using DataCallback = delegate<void()>;
    /**
     * @brief      Event when data is received.
     *             Does not push data, just signals its presence.
     *
     * @param[in]  cb    The callback
     */
    virtual void on_data(DataCallback cb) = 0;

    /**
     * @return The size of the next available chunk of data if any.
     */
    virtual size_t next_size() = 0;

    /**
     * @return The next available chunk of data if any.
     */
    virtual buffer_t read_next() = 0;

    /** Called with nothing ¯\_(ツ)_/¯ */
    using CloseCallback = delegate<void()>;
    /**
     * @brief      Event for when the Stream is being closed.
     *
     * @param[in]  cb    The close callback
     */
    virtual void on_close(CloseCallback cb) = 0;

    /** Called with the number of bytes written. */
    using WriteCallback = delegate<void(size_t)>;
    /**
     * @brief      Event for when data has been written.
     *
     * @param[in]  cb    The write callback
     */
    virtual void on_write(WriteCallback cb) = 0;

    /**
     * @brief      Async write of a data with a length.
     *
     * @param[in]  buf   data
     * @param[in]  n     length
     */
    virtual void write(const void* buf, size_t n) = 0;

    /**
     * @brief      Async write of a shared buffer with a length.
     *
     * @param[in]  buffer  shared buffer
     * @param[in]  n       length
     */
    virtual void write(buffer_t buf) = 0;

    /**
     * @brief      Async write of a string.
     *
     * @param[in]  str   The string
     */
    virtual void write(const std::string& str) = 0;

    /**
     * @brief      Closes the stream.
     */
    virtual void close() = 0;

    /**
     * @brief      Resets all callbacks.
     */
    virtual void reset_callbacks() = 0;

    /**
     * @brief      Returns the streams local socket.
     *
     * @return     A TCP Socket
     */
    virtual Socket local() const = 0;

    /**
     * @brief      Returns the streams remote socket.
     *
     * @return     A TCP Socket
     */
    virtual Socket remote() const = 0;

    /**
     * @brief      Returns a string representation of the stream.
     *
     * @return     String representation of the stream.
     */
    virtual std::string to_string() const = 0;

    /**
     * @brief      Determines if connected (established).
     *
     * @return     True if connected, False otherwise.
     */
    virtual bool is_connected() const noexcept = 0;

    /**
     * @brief      Determines if writable. (write is allowed)
     *
     * @return     True if writable, False otherwise.
     */
    virtual bool is_writable() const noexcept = 0;

    /**
     * @brief      Determines if readable. (data can be received)
     *
     * @return     True if readable, False otherwise.
     */
    virtual bool is_readable() const noexcept = 0;

    /**
     * @brief      Determines if closing.
     *
     * @return     True if closing, False otherwise.
     */
    virtual bool is_closing() const noexcept = 0;

    /**
     * @brief      Determines if closed.
     *
     * @return     True if closed, False otherwise.
     */
    virtual bool is_closed() const noexcept = 0;

    /**
     * Returns the CPU id the Stream originates from
    **/
    virtual int get_cpuid() const noexcept = 0;

    /**
     * Returns the underlying transport, or nullptr if bottom.
     * If no transport present, most likely its a TCP stream, in which
     * case you can dynamic_cast and call tcp() to get the connection
    **/
    virtual Stream* transport() noexcept = 0;

    /** Recursively navigate to the transport stream at the bottom **/
    inline Stream* bottom_transport() noexcept;

    /** default empty implementation of serialize_to(...) **/
    virtual size_t serialize_to(void*, size_t) const {
      throw std::runtime_error("Not implemented for this stream");
    }
    /** default subid for stream **/
    virtual uint16_t serialization_subid() const {
      // NOTE: when provided with nullptr and size == 0, return an id
      throw std::runtime_error("Not implemented for this stream");
    }

    virtual ~Stream() = default;
  }; // < class Stream

  inline Stream* Stream::bottom_transport() noexcept
  {
    Stream* stream = this;
    while (stream->transport() != nullptr) {
        stream = stream->transport();
    }
    return stream;
  }

} // < namespace net

#endif // < NET_STREAM_HPP
