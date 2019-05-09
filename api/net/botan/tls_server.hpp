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
#ifndef NET_TLS_SERVER_STREAM_HPP
#define NET_TLS_SERVER_STREAM_HPP

#include <botan/credentials_manager.h>
#include <botan/rng.h>
#include <botan/tls_server.h>
#include <botan/tls_callbacks.h>
#include <net/tcp/connection.hpp>
#include <net/botan/credman.hpp>

namespace net
{
namespace botan
{
class Server : public Botan::TLS::Callbacks, public net::Stream
{
public:
  Server(net::Stream_ptr remote,
         Botan::RandomNumberGenerator& rng,
         Botan::Credentials_Manager& credman)
  : m_creds{credman},
    m_session_manager{},
    m_tls{*this, m_session_manager, m_creds, m_policy, rng},
    m_transport{std::move(remote)}
  {
    assert(m_transport->is_connected());
    // default read callback
    m_transport->on_read(4096, {this, &Server::tls_read});
    m_transport->on_close({this, &Server::close_callback_once});
  }
  ~Server() {
    assert(m_busy == false && "Cannot delete stream while in its call stack");
  }

  void on_read(size_t bs, ReadCallback cb) override
  {
    m_transport->on_read(bs, {this, &Server::tls_read});
    this->m_on_read = cb;
  }
  void on_data(DataCallback cb) override {
    // FIXME
    throw std::runtime_error("on_data not implemented on botan::server");
  }
  size_t next_size() override {
    // FIXME
    throw std::runtime_error("next_size not implemented on botan::server");
  }
  buffer_t read_next() override {
    // FIXME
    throw std::runtime_error("read_next not implemented on botan::server");
  }
  void on_write(WriteCallback cb) override {
    this->m_on_write = cb;
  }
  void on_connect(ConnectCallback cb) override {
    this->m_on_connect = cb;
  }
  void on_close(CloseCallback cb) override {
    this->m_on_close = cb;
  }

  void write(const void* buf, size_t n) override
  {
    m_tls.send((uint8_t*) buf, n);
  }
  void write(const std::string& str) override
  {
    this->write(str.data(), str.size());
  }
  void write(buffer_t buf) override
  {
    m_tls.send(buf->data(), buf->size());
  }

  void close() override
  {
    if (this->m_busy) {
      this->m_deferred_close = true; return;
    }
    CloseCallback cb = std::move(m_on_close);
    this->reset_callbacks();
    m_transport->close();
    if (cb) cb();
  }
  void close_callback_once()
  {
    CloseCallback cb = std::move(m_on_close);
    this->reset_callbacks();
    if (cb) cb();
  }
  void reset_callbacks() override
  {
    m_on_read  = nullptr;
    m_on_write = nullptr;
    m_on_connect = nullptr;
    m_on_close = nullptr;
  }

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
    return m_transport->is_connected();
  }
  bool is_writable() const noexcept override {
    return m_tls.is_active();
  }
  bool is_readable() const noexcept override {
    return m_transport->is_readable();
  }
  bool is_closing() const noexcept override {
    return m_transport->is_closing();
  }
  bool is_closed() const noexcept override {
    return m_tls.is_closed() || m_transport->is_closed();
  }

  int get_cpuid() const noexcept override {
    return m_transport->get_cpuid();
  }

  Stream* transport() noexcept override {
    return m_transport.get();
  }

protected:
  void tls_read(buffer_t buf)
  {
    this->m_busy = true;
    try
    {
      int rem = m_tls.received_data(buf->data(), buf->size());
      (void) rem;
      //printf("Finished processing (rem: %u)\n", rem);
    }
    catch(Botan::Exception& e)
    {
      printf("Fatal TLS error %s\n", e.what());
      this->close();
    }
    this->m_busy = false;
    if (this->m_deferred_close) this->close();
  }

  void tls_alert(Botan::TLS::Alert alert) override
  {
    // ignore close notifications
    if (alert.type() != Botan::TLS::Alert::CLOSE_NOTIFY)
    {
      printf("Got a %s alert: %s\n",
            (alert.is_fatal() ? "fatal" : "warning"),
            alert.type_string().c_str());
    }
  }

  bool tls_session_established(const Botan::TLS::Session&) override
  {
    // return true to store session
    return true;
  }

  void tls_emit_data(const uint8_t buf[], size_t len) override
  {
    m_transport->write(buf, len);
    if (m_on_write) {
      this->m_busy = true;
      m_on_write(len);
      this->m_busy = false;
      if (this->m_deferred_close) this->close();
    }
  }

  void tls_record_received(uint64_t, const uint8_t buf[], size_t buf_len) override
  {
    if (m_on_read) {
      m_on_read(Stream::construct_buffer(buf, buf + buf_len));
    }
  }

  void tls_session_activated() override
  {
    if (m_on_connect) m_on_connect(*this);
  }

private:
  Stream::ReadCallback    m_on_read = nullptr;
  Stream::WriteCallback   m_on_write = nullptr;
  Stream::ConnectCallback m_on_connect = nullptr;
  Stream::CloseCallback   m_on_close = nullptr;
  bool m_busy = false;
  bool m_deferred_close = false;

  Botan::Credentials_Manager&   m_creds;
  Botan::TLS::Strict_Policy     m_policy;
  Botan::TLS::Session_Manager_Noop m_session_manager;

  Botan::TLS::Server m_tls;
  net::Stream_ptr    m_transport = nullptr;
};

} // tls
} // net

#endif
