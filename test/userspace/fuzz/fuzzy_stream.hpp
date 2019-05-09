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
#include <kernel/events.hpp>
#include <deque>

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
    using Stream_ptr = std::unique_ptr<Stream>;
    // read callback for when data is going out on this stream
    Stream(net::Socket, net::Socket, ReadCallback, bool async = false);
    virtual ~Stream();
    // call this with testdata
    void give_payload(buffer_t);
    // enable async behavior (to avoid trashing buffers)
    void enable_async();
    // for when we want to close fast
    void transport_level_close();

    //** ALL functions below are used by higher layers **//
    void write(buffer_t buffer) override;
    void write(const std::string&) override;
    void write(const void* buf, size_t n) override;
    void close() override;
    void reset_callbacks() override;

    void on_data(DataCallback cb) override {
      (void) cb;
      printf("fuzzy stream %p on_data\n", this);
    }

    size_t next_size() override {
      if (is_async() && !m_async_queue.empty()) {
        const size_t size = m_async_queue.front()->size();
        printf("fuzzy stream %p next_size -> %zu\n", this, size);
        return size;
      }
      printf("fuzzy stream %p next_size -> 0 (empty)\n", this);
      return 0;
    }

    buffer_t read_next() override {
      if (is_async() && !m_async_queue.empty()) {
        auto buf = std::move(m_async_queue.front());
        printf("fuzzy stream %p read_next -> %zu\n", this, buf->size());
        m_async_queue.pop_front();
        return buf;
      }
      printf("fuzzy stream %p read_next -> empty\n", this);
      return nullptr;
    }

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
      return !m_is_closed;
    }
    bool is_writable() const noexcept override {
      return !m_is_closed;
    }
    bool is_readable() const noexcept override {
      return !m_is_closed;
    }
    bool is_closing() const noexcept override {
      return m_is_closed;
    }
    bool is_closed() const noexcept override {
      return m_is_closed;
    }
    int get_cpuid() const noexcept override {
      return 0;
    }
    net::Stream* transport() noexcept override {
      assert(0);
    }

  private:
    void close_callback_once();
    bool is_async() const noexcept { return this->m_async_event != 0; }

    net::Socket m_local;
    net::Socket m_remote;
    delegate<void(buffer_t)> m_payload_out = nullptr;

    bool    m_busy = false;
    bool    m_deferred_close = false;
    bool    m_is_closed    = false;
    uint8_t m_async_event  = 0;
    std::deque<buffer_t> m_async_queue;
    ConnectCallback  m_on_connect = nullptr;
    ReadCallback     m_on_read    = nullptr;
    WriteCallback    m_on_write   = nullptr;
    CloseCallback    m_on_close   = nullptr;
  };
  using Stream_ptr = Stream::Stream_ptr;

  inline Stream::Stream(net::Socket lcl, net::Socket rmt,
                        ReadCallback payload_out, const bool async)
    : m_local(lcl), m_remote(rmt), m_payload_out(payload_out)
  {
    if (async) {
      this->m_async_event = Events::get().subscribe(
        [this] () {
          auto copy = std::move(this->m_async_queue);
          assert(this->m_async_queue.empty());
          for (auto& buffer : copy) {
              FZS_PRINT("fuzzy::Stream::async_write(%p: %p, %zu)\n",
                        this, buffer->data(), buffer->size());
              this->m_payload_out(std::move(buffer));
          }
        });
      assert(this->is_async());
    }
  }
  inline Stream::~Stream()
  {
    FZS_PRINT("fuzzy::Stream::~Stream(%p)\n", this);
    assert(m_busy == false && "Cannot delete stream while in its call stack");
    if (!this->is_closed()) {
      this->transport_level_close();
    }
  }

  inline void Stream::write(buffer_t buffer)
  {
    if (not this->is_async()) {
        FZS_PRINT("fuzzy::Stream::write(%p: %p, %zu)\n",
                  this, buffer->data(), buffer->size());
        this->m_payload_out(std::move(buffer));
    }
    else {
        FZS_PRINT("fuzzy::Stream::write(%p: %p, %zu) ASYNC\n",
                  this, buffer->data(), buffer->size());
        Events::get().trigger_event(this->m_async_event);
        this->m_async_queue.push_back(std::move(buffer));
    }
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
    FZS_PRINT("fuzzy::Stream::internal_read(%p, %zu)\n", this, data_in->size());
    if (this->m_on_read) {
      assert(this->m_busy == false);
      this->m_busy = true;
      this->m_on_read(data_in);
      this->m_busy = false;
    }
  }
  inline void Stream::transport_level_close()
  {
    CloseCallback callback = std::move(this->m_on_close);
    if (callback) callback();
  }

  inline void Stream::close()
  {
    FZS_PRINT("fuzzy::Stream::close(%p)\n", this);
    if (this->m_busy) {
      this->m_deferred_close = true; return;
    }
    CloseCallback func = std::move(this->m_on_close);
    // unsubscribe and disable async writes
    if (this->m_async_event) {
        Events::get().unsubscribe(this->m_async_event);
        this->m_async_event = 0;
    }
    this->m_is_closed = true;
    this->reset_callbacks();
    if (func) func();
  }
  inline void Stream::close_callback_once()
  {
    FZS_PRINT("fuzzy::Stream::close_callback_once()\n");
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
} // fuzzy
