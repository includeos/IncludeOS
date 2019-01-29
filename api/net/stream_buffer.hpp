// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2016-2017 Oslo and Akershus University College of Applied Sciences
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
#ifndef STREAMBUFFERR_HPP
#define STREAMBUFFERR_HPP

#include <net/stream.hpp>
#include <queue>
#include <util/timer.hpp>

namespace net
{
  class StreamBuffer : public net::Stream
  {
  public:
    StreamBuffer(Timers::duration_t timeout=std::chrono::microseconds(10))
      : timer({this,&StreamBuffer::congested}),congestion_timeout(timeout) {}
    using buffer_t = os::mem::buf_ptr;

    virtual ~StreamBuffer() {
      timer.stop();
    }

    void on_connect(ConnectCallback cb) override {
      m_on_connect = std::move(cb);
    }

    void on_read(size_t, ReadCallback cb) override {
      m_on_read = std::move(cb);
      signal_data();
    }
    void on_data(DataCallback cb) override {
      m_on_data = std::move(cb);
      signal_data();
    }
    size_t next_size() override;

    buffer_t read_next() override;

    void on_close(CloseCallback cb) override {
      m_on_close = std::move(cb);
    }
    void on_write(WriteCallback cb) override {
      m_on_write = std::move(cb);
    }

    void signal_data();

    bool read_congested() const noexcept
    { return m_read_congested; }

    bool write_congested() const noexcept
    { return m_write_congested; }

    /**
     * @brief Construct a shared read vector used by streams
     * If allocation failed congestion flag is set
     *
     * @param construction parameters
     *
     * @return nullptr on failure, shared_ptr to buffer on success
     */
    template <typename... Args>
    buffer_t construct_read_buffer(Args&&... args)
    {
      return construct_buffer_with_flag(m_read_congested,std::forward<Args> (args)...);
    }

    /**
     * @brief Construct a shared write vector used by streams
     * If allocation failed congestion flag is set
     *
     * @param construction parameters
     *
     * @return nullptr on failure, shared_ptr to buffer on success
     */
    template <typename... Args>
    buffer_t construct_write_buffer(Args&&... args)
    {
      return construct_buffer_with_flag(m_write_congested,std::forward<Args> (args)...);
    }

    virtual void handle_read_congestion() = 0;
    virtual void handle_write_congestion() = 0;
  protected:
    void closed()
    { if (m_on_close) m_on_close(); }
    void connected()
    { if (m_on_connect) m_on_connect(*this); }
    void stream_on_write(int n)
    { if (m_on_write) m_on_write(n); }
    void stream_on_read(buffer_t buffer)
    { if (m_on_read) m_on_read(std::move(buffer)); }
    void enqueue_data(buffer_t data)
    { m_send_buffers.push_back(data); }

    void congested();

    CloseCallback getCloseCallback() { return std::move(this->m_on_close); }

    void reset_callbacks() override
    {
      //remove queue and reset congestion flags and busy flag ??
      this->m_on_close = nullptr;
      this->m_on_connect = nullptr;
      this->m_on_read  = nullptr;
      this->m_on_write = nullptr;
      this->m_on_data = nullptr;
    }
    Timer timer;

  private:
    ConnectCallback  m_on_connect = nullptr;
    ReadCallback     m_on_read    = nullptr;
    DataCallback     m_on_data    = nullptr;
    WriteCallback    m_on_write   = nullptr;
    CloseCallback    m_on_close   = nullptr;
    std::deque<buffer_t> m_send_buffers;
    Timer::duration_t congestion_timeout;
    bool m_write_congested = false;
    bool m_read_congested  = false;

    /**
     * @brief Construct a shared vector and set congestion flag if allocation fails
     *
     * @param flag the flag to set true or false on allocation failure
     * @param args arguments to constructing the buffer
     * @return nullptr on failure , shared pointer to buffer on success
     */

    template <typename... Args>
    buffer_t construct_buffer_with_flag(bool &flag,Args&&... args)
    {
      try
      {
        auto buffer = std::make_shared<os::mem::buffer>(std::forward<Args> (args)...);
        flag = false;
        return buffer;
      }
      catch (std::bad_alloc &e)
      {
        flag = true;
        timer.start(congestion_timeout);
        return nullptr;
      }
    }
  }; // < class StreamBuffer

  inline size_t StreamBuffer::next_size()
  {
    if (not m_send_buffers.empty()) {
      return m_send_buffers.front()->size();
    }
    return 0;
  }

  inline StreamBuffer::buffer_t StreamBuffer::read_next()
  {
    if (not m_send_buffers.empty()) {
      auto buf = m_send_buffers.front();
      m_send_buffers.pop_front();
      return buf;
    }
    return nullptr;
  }

  inline void StreamBuffer::congested()
  {
    if (m_read_congested)
    {
      handle_read_congestion();
    }
    if (m_write_congested)
    {
      handle_write_congestion();
    }
    //if any of the congestion states are still active make sure the timer is running
    if(m_read_congested or m_write_congested)
    {
      if (!timer.is_running())
      {
        timer.start(congestion_timeout);
      }
    }
    else
    {
      if (timer.is_running())
      {
        timer.stop();
      }
    }
  }

  inline void StreamBuffer::signal_data()
  {
    if (not m_send_buffers.empty())
    {
      if (m_on_data != nullptr){
        //on_data_callback();
        m_on_data();
        if (not m_send_buffers.empty()) {
          m_read_congested=true;
          timer.start(congestion_timeout);
        }
      }
      else if (m_on_read != nullptr)
      {
        for (auto buf : m_send_buffers) {
          // Pop each time, in case callback leads to another call here.
          m_send_buffers.pop_front();
          m_on_read(buf);
          if (m_on_read == nullptr) {
            break;
          } //if calling m_on_read reset the callbacks exit
        }
      }
    }
  }
} // namespace net

#endif // STREAM_BUFFER_HPP
