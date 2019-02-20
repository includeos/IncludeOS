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
#include <net/stream_buffer.hpp>
#include <s2n.h>
#include <util/ringbuffer.hpp>
#include <errno.h>

//#define VERBOSE_S2N
#ifdef VERBOSE_S2N
#define S2N_PRINT(fmt, ...) printf(fmt, ##__VA_ARGS__)
#else
#define S2N_PRINT(fmt, ...) /* fmt */
#endif
#define S2N_BUSY(func, ...) \
    { bool b = this->m_busy; \
      this->m_busy = true;   \
      func(__VA_ARGS__);     \
      this->m_busy = b;}

extern "C" int     s2n_connection_handshake_complete(struct s2n_connection*);
extern "C" ssize_t s2n_conn_serialize_to(struct s2n_connection*, void* addr, size_t);
extern "C" struct s2n_connection* s2n_conn_deserialize_from(struct s2n_config* config, const void* addr, const size_t);


namespace s2n
{
  typedef int s2n_connection_send(void *io_context, const uint8_t *buf, uint32_t len);
  typedef int s2n_connection_recv(void *io_context, uint8_t *buf, uint32_t len);
  static inline s2n_connection_send s2n_send;
  static inline s2n_connection_recv s2n_recv;

  static void print_s2n_error(const char* app_error)
  {
      fprintf(stderr, "%s: '%s' : '%s'\n",
              app_error,
              s2n_strerror(s2n_errno, "EN"),
              s2n_strerror_debug(s2n_errno, "EN"));
  }

  struct TLS_stream : public net::StreamBuffer
  {
    static const uint16_t SUBID = 21476;
    using TLS_stream_ptr = std::unique_ptr<TLS_stream>;
    using Stream_ptr = net::Stream_ptr;

    TLS_stream(s2n_config*, Stream_ptr, bool outgoing = false);
    TLS_stream(s2n_connection*, Stream_ptr, bool outgoing = false);
    virtual ~TLS_stream();

    void write(buffer_t buffer) override;
    void write(const std::string&) override;
    void write(const void* buf, size_t n) override;
    void close() override;

    net::Socket local() const override {
      return m_transport->local();
    }
    net::Socket remote() const override {
      return m_transport->remote();
    }
    std::string to_string() const override {
      return m_transport->to_string();
    }

    bool is_connected() const noexcept override {
      return handshake_completed() && m_transport->is_connected();
    }
    bool is_writable() const noexcept override {
      return is_connected() && m_transport->is_writable();
    }
    bool is_readable() const noexcept override {
      return m_transport->is_readable();
    }
    bool is_closing() const noexcept override {
      return m_transport->is_closing();
    }
    bool is_closed() const noexcept override {
      return m_transport->is_closed();
    }

    int get_cpuid() const noexcept override {
      return m_transport->get_cpuid();
    }

    Stream* transport() noexcept override {
      return m_transport.get();
    }

    size_t   serialize_to(void*, size_t) const override;
    uint16_t serialization_subid() const override { return SUBID; }
    static TLS_stream_ptr deserialize_from(s2n_config*  config,
                                           Stream_ptr   transport,
                                           const bool   outgoing,
                                           const void*  data,
                                           const size_t size);

  private:
    void initialize(bool outgoing);
    void tls_read(buffer_t);
    int  tls_write(const uint8_t*, uint32_t len);
    bool handshake_completed() const noexcept;
    void close_callback_once();
    void handle_read_congestion() override;
    void handle_write_congestion() override;

    Stream_ptr m_transport = nullptr;
    s2n_connection* m_conn = nullptr;
    bool  m_busy = false;
    bool  m_deferred_close = false;
    FixedRingBuffer<16384> m_readq;

    friend s2n_connection_recv s2n_recv;
    friend s2n_connection_send s2n_send;
  };
  using TLS_stream_ptr = TLS_stream::TLS_stream_ptr;

  inline TLS_stream::TLS_stream(s2n_config* config, Stream_ptr t,
                                const bool outgoing)
    : m_transport(std::move(t))
  {
    assert(this->m_transport != nullptr);
    this->m_conn = s2n_connection_new(outgoing ? S2N_CLIENT : S2N_SERVER);
    if (s2n_connection_set_config(this->m_conn, config) < 0) {
      print_s2n_error("Error setting config");
      throw std::runtime_error("Error setting s2n::TLS_stream config");
    }
    this->initialize(outgoing);
  }
  inline TLS_stream::TLS_stream(s2n_connection* conn, Stream_ptr t,
                                const bool outgoing)
    : m_transport(std::move(t)), m_conn(conn)
  {
    assert(this->m_transport != nullptr);
    assert(this->m_conn != nullptr);
    this->initialize(outgoing);
  }
  inline void TLS_stream::initialize(bool outgoing)
  {
    s2n_connection_set_send_cb(this->m_conn, s2n_send);
    s2n_connection_set_recv_cb(this->m_conn, s2n_recv);
    s2n_connection_set_send_ctx(this->m_conn, this);
    s2n_connection_set_recv_ctx(this->m_conn, this);
    s2n_connection_set_ctx(this->m_conn, this);
    this->m_transport->on_read(8192, {this, &TLS_stream::tls_read});

    // initial handshake on outgoing connections
    if (outgoing)
    {
      s2n_blocked_status blocked;
      int r = s2n_negotiate(this->m_conn, &blocked);
      S2N_PRINT("s2n_negotiate: %d / %d, blocked = %x\n",
                r, m_readq.size(), blocked);
      if (r < 0) {
        if (s2n_error_get_type(s2n_errno) != S2N_ERR_T_BLOCKED)
        {
          fprintf(stderr, "Failed to negotiate: '%s'. %s\n", s2n_strerror(s2n_errno, "EN"), s2n_strerror_debug(s2n_errno, "EN"));
          fprintf(stderr, "Alert: %d\n", s2n_connection_get_alert(this->m_conn));
          this->close();
        }
        return;
      }
    }
  }
  inline TLS_stream::~TLS_stream()
  {
    S2N_PRINT("s2n::TLS_stream::~TLS_stream(%p)\n", this);
    assert(m_busy == false && "Cannot delete stream while in its call stack");
    s2n_connection_free(this->m_conn);
  }

  inline void TLS_stream::write(buffer_t buffer)
  {
    this->write(buffer->data(), buffer->size());
  }
  inline void TLS_stream::write(const std::string& str)
  {
    this->write(str.data(), str.size());
  }
  inline void TLS_stream::write(const void* data, const size_t len)
  {
    assert(handshake_completed());
    auto* buf = static_cast<const uint8_t*> (data);
    s2n_blocked_status blocked;
    int res = ::s2n_send(this->m_conn, buf, len, &blocked);
    S2N_PRINT("write %zu bytes -> %d, blocked = %x\n", len, res, blocked);
    (void) res;
  }

  inline void TLS_stream::tls_read(buffer_t data_in)
  {
    if (data_in != nullptr) {
      S2N_PRINT("s2n::tls_read(%p): %p, %zu bytes -> %p\n",
                this, data_in->data(), data_in->size(), m_readq.data());
      m_readq.write(data_in->data(), data_in->size());
    }

    s2n_blocked_status blocked;
    do {
      int r = 0;
      if (handshake_completed())
      {
        auto buffer = StreamBuffer::construct_read_buffer(8192);
        if (buffer == nullptr) return; // what else is there to do?

        r = s2n_recv(this->m_conn, buffer->data(), buffer->size(), &blocked);
        S2N_PRINT("s2n_recv: %d, blocked = %x\n", r, blocked);
        if (r > 0) {
          buffer->resize(r);
          this->enqueue_data(buffer);
          this->signal_data();
        }
        else if (r == 0) {
          // normal peer shutdown
          this->close();
          return;
        }
        else if (r < 0) {
          if (s2n_error_get_type(s2n_errno) != S2N_ERR_T_BLOCKED)
          {
            fprintf(stderr, "Failed to negotiate: '%s'. %s\n", s2n_strerror(s2n_errno, "EN"), s2n_strerror_debug(s2n_errno, "EN"));
            fprintf(stderr, "Alert: %d\n", s2n_connection_get_alert(this->m_conn));
            this->close();
          }
          return;
        }
      } else {
        r = s2n_negotiate(this->m_conn, &blocked);
        S2N_PRINT("s2n_negotiate: %d / %d, blocked = %x\n",
                  r, m_readq.size(), blocked);
        if (r == 0) {
          this->connected();
        }
        else if (r < 0) {
          if (s2n_error_get_type(s2n_errno) != S2N_ERR_T_BLOCKED)
          {
            fprintf(stderr, "Failed to negotiate: '%s'. %s\n", s2n_strerror(s2n_errno, "EN"), s2n_strerror_debug(s2n_errno, "EN"));
            fprintf(stderr, "Alert: %d\n", s2n_connection_get_alert(this->m_conn));
            this->close();
          }
          return;
        }
      }
    } while (blocked != S2N_NOT_BLOCKED);
  }
  int s2n_recv(void* ctx, uint8_t* buf, uint32_t len)
  {
    auto* self = (TLS_stream*) ctx;
    if ((uint32_t) self->m_readq.size() < len) {
      S2N_PRINT("s2n_recv(%p): %p, %u = BLOCKED\n", ctx, buf, len);
      errno = EWOULDBLOCK;
      return -1;
    }
    int res = self->m_readq.read((char*) buf, len);
    S2N_PRINT("s2n_recv(%p): %p, %u = %d\n", ctx, buf, len, res);
    return res;
  }
  int s2n_send(void* ctx, const uint8_t* buf, uint32_t len)
  {
    S2N_PRINT("s2n_send(%p): %p, %u\n", ctx, buf, len);
    return ((TLS_stream*) ctx)->tls_write(buf, len);
  }
  inline int TLS_stream::tls_write(const uint8_t* buf, uint32_t len)
  {
    auto buffer = StreamBuffer::construct_write_buffer(buf, buf + len);
    if (buffer != nullptr) {
      this->m_transport->write(std::move(buffer));
      S2N_BUSY(StreamBuffer::stream_on_write, len);
      return len;
    }
    errno = EWOULDBLOCK;
    return -1;
  }

  inline void TLS_stream::handle_read_congestion()
  {
    S2N_PRINT("s2n::handle_read_congestion() calling tls_read\n");
    this->tls_read(nullptr);

    S2N_BUSY(StreamBuffer::signal_data);

    if (this->m_deferred_close) {
      S2N_PRINT("s2n::read() close after tls_read\n");
      this->close();
      return;
    }
  }
  inline void TLS_stream::handle_write_congestion()
  {
    S2N_PRINT("s2n::handle_write_congestion() what now?\n");
    //while(tls_write(nullptr, 0) >  0);
  }

  inline void TLS_stream::close()
  {
    S2N_PRINT("s2n::close(%p)\n", this);
    if (this->m_busy) {
      this->m_deferred_close = true; return;
    }
    CloseCallback func = this->getCloseCallback();
    this->reset_callbacks();
    if (m_transport->is_connected())
        m_transport->close();
    if (func) func();
  }
  inline void TLS_stream::close_callback_once()
  {
    if (this->m_busy) {
      this->m_deferred_close = true; return;
    }
    CloseCallback func = this->getCloseCallback();
    this->reset_callbacks();
    if (func) func();
  }

  inline bool TLS_stream::handshake_completed() const noexcept
  {
    return s2n_connection_handshake_complete(this->m_conn);
  }

  struct serialized_stream {
    ssize_t conn_size  = 0;
    char next[0];

    void* conn_addr() {
      return &next[0];
    }

    size_t size() const noexcept {
      return sizeof(serialized_stream) + conn_size;
    }
  };

  inline size_t TLS_stream::serialize_to(void* addr, size_t size) const
  {
    assert(addr != nullptr && size > sizeof(serialized_stream));
    // create header
    auto* hdr = (serialized_stream*) addr;
    *hdr = {};
    // subtract size of header
    size -= sizeof(serialized_stream);
    // serialize connection and set size from result
    hdr->conn_size = s2n_conn_serialize_to(this->m_conn, hdr->conn_addr(), size);
    if (hdr->conn_size < 0) {
        throw std::runtime_error("Failed to serialize TLS connection");
    }
    return hdr->size();
  }

  inline TLS_stream_ptr
  TLS_stream::deserialize_from(s2n_config*  config,
                               Stream_ptr   transport,
                               const bool   outgoing,
                               const void*  data,
                               const size_t size)
  {
    auto* hdr = (serialized_stream*) data;
    if (size != hdr->size()) {
        throw std::runtime_error("TLS serialization size mismatch");
    }
    // restore connection
    auto* conn = s2n_conn_deserialize_from(config, hdr->conn_addr(), hdr->conn_size);
    if (conn != nullptr) {
      // restore stream
      return std::make_unique<TLS_stream> (conn, std::move(transport), outgoing);
    }
    return nullptr;
  }
} // s2n
