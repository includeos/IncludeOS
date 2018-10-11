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
#include <net/stream.hpp>

//#define VERBOSE_FUZZY_STREAM
#ifdef VERBOSE_FUZZY_STREAM
#define FZS_PRINT(fmt, ...) printf(fmt, ##__VA_ARGS__)
#else
#define FZS_PRINT(fmt, ...) /* fmt */
#endif

namespace fuzzy
{
  struct Stream : public net::Stream
  {
    using Stream_ptr = net::Stream_ptr;

    // read callback for when data is going out on this stream
    Stream(net::Socket, net::Socket, ReadCallback);
    virtual ~Stream();
    // call this with testdata
    void give_payload(buffer_t);
    void transport_level_close();

    //** ALL functions below are used by higher layers **//
    void write(buffer_t buffer) override;
    void write(const std::string&) override;
    void write(const void* buf, size_t n) override;
    void close() override;
    void reset_callbacks() override;

    net::Socket local() const override {
      return m_local;
    }
    net::Socket remote() const override {
      return m_remote;
    }
    std::string to_string() const override {
      return "Local: " + local().to_string()
          + " Remote: " + remote().to_string();
    }

    void on_connect(ConnectCallback cb) override {
      m_on_connect = std::move(cb);
    }
    void on_read(size_t, ReadCallback cb) override {
      m_on_read = std::move(cb);
    }
    void on_close(CloseCallback cb) override {
      m_on_close = std::move(cb);
    }
    void on_write(WriteCallback cb) override {
      m_on_write = std::move(cb);
    }

    bool is_connected() const noexcept override {
      return true;
    }
    bool is_writable() const noexcept override {
      return true;
    }
    bool is_readable() const noexcept override {
      return true;
    }
    bool is_closing() const noexcept override {
      return false;
    }
    bool is_closed() const noexcept override {
      return false;
    }
    int get_cpuid() const noexcept override {
      return 0;
    }
    net::Stream* transport() noexcept override {
      assert(0);
    }
    size_t serialize_to(void*) const override;

  private:
    void close_callback_once();

    net::Socket m_local;
    net::Socket m_remote;
    delegate<void(buffer_t)> m_payload_out = nullptr;
    
    bool  m_busy = false;
    bool  m_deferred_close = false;
    ConnectCallback  m_on_connect = nullptr;
    ReadCallback     m_on_read    = nullptr;
    WriteCallback    m_on_write   = nullptr;
    CloseCallback    m_on_close   = nullptr;
  };
  using Stream_ptr = Stream::Stream_ptr;

  inline Stream::Stream(net::Socket lcl, net::Socket rmt,
                        ReadCallback payload_out)
    : m_local(lcl), m_remote(rmt), m_payload_out(payload_out) {}
  inline Stream::~Stream()
  {
    assert(m_busy == false && "Cannot delete stream while in its call stack");
  }

  inline void Stream::write(buffer_t buffer)
  {
    FZS_PRINT("fuzzy::Stream::write(%zu)\n", buffer->size());
    this->m_payload_out(std::move(buffer));
  }
  inline void Stream::write(const std::string& str)
  {
    this->write(construct_buffer(str.data(), str.data() + str.size()));
  }
  inline void Stream::write(const void* data, const size_t len)
  {
    const auto* buffer = (const char*) data;
    this->write(construct_buffer(buffer, buffer + len));
  }

  inline void Stream::give_payload(buffer_t data_in)
  {
    FZS_PRINT("fuzzy::Stream::internal_read(%zu)\n", data_in->size());
    if (this->m_on_read) {
      this->m_busy = true;
      this->m_on_read(data_in);
      this->m_busy = false;
    }
  }
  inline void Stream::transport_level_close()
  {
    if (this->m_on_close) this->m_on_close();
  }

  inline void Stream::close()
  {
    if (this->m_busy) {
      this->m_deferred_close = true; return;
    }
    CloseCallback func = std::move(this->m_on_close);
    this->reset_callbacks();
    if (this->is_connected())
        this->close();
    if (func) func();
  }
  inline void Stream::close_callback_once()
  {
    if (this->m_busy) {
      this->m_deferred_close = true; return;
    }
    CloseCallback func = std::move(this->m_on_close);
    this->reset_callbacks();
    if (func) func();
  }
  inline void Stream::reset_callbacks()
  {
    this->m_on_close = nullptr;
    this->m_on_connect = nullptr;
    this->m_on_read  = nullptr;
    this->m_on_write = nullptr;
  }
  
  size_t Stream::serialize_to(void*) const {
    return 0;
  }
} // s2n
