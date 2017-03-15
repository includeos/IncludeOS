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
#include <net/tls/credman.hpp>

namespace net
{
namespace tls
{
class Server : public Botan::TLS::Callbacks, public tcp::Stream
{
public:
  using Connection_ptr = tcp::Connection_ptr;


  Server(Connection_ptr remote,
             Botan::RandomNumberGenerator& rng,
             Botan::Credentials_Manager& credman) :
    tcp::Stream({remote}),
    m_rng(rng),
    m_creds(credman),
    m_session_manager(m_rng),
    m_tls(*this, m_session_manager, m_creds, m_policy, m_rng)
  {
    assert(tcp->is_connected());
    // default read callback
    tcp->on_read(4096, {this, &Server::tls_read});
  }

  void on_read(size_t bs, ReadCallback cb) override
  {
    tcp->on_read(bs, {this, &Server::tls_read});
    this->o_read = cb;
  }
  void on_write(WriteCallback cb) override
  {
    this->o_write = cb;
  }
  void on_connect(ConnectCallback cb) override
  {
    this->o_connect = cb;
  }

  void write(const void* buf, size_t n) override
  {
    m_tls.send((uint8_t*) buf, n);
  }
  void write(const std::string& str) override
  {
    this->write(str.data(), str.size());
  }
  void write(Chunk ch) override
  {
    m_tls.send(ch.data(), ch.size());
  }
  void write(buffer_t buf, size_t n) override
  {
    m_tls.send(buf.get(), n);
  }

  std::string to_string() const override {
    return tcp->to_string();
  }

  void reset_callbacks() override
  {
    o_connect = nullptr;
    o_read  = nullptr;
    o_write = nullptr;
    tcp->reset_callbacks();
  }

protected:
  void tls_read(buffer_t buf, const size_t n)
  {
    this->tls_receive(buf.get(), n);
  }

  void tls_receive(const uint8_t* buf, const size_t n)
  {
    try
    {
      int rem = m_tls.received_data(buf, n);
      (void) rem;
      //printf("Finished processing (rem: %u)\n", rem);
    }
    catch(Botan::Exception& e)
    {
      printf("Fatal TLS error %s\n", e.what());
      this->close();
    }
    catch(...)
    {
      printf("Unknown error!\n");
      this->close();
    }
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
    tcp->write(buf, len);
  }

  void tls_record_received(uint64_t, const uint8_t buf[], size_t buf_len) override
  {
    if (o_read)
    {
      auto buffff = std::shared_ptr<uint8_t> (new uint8_t[buf_len]);
      memcpy(buffff.get(), buf, buf_len);
      
      o_read(buffff, buf_len);
    }
  }

  void tls_session_activated() override
  {
    if (o_connect) o_connect(*this);
  }

private:
  Stream::ReadCallback    o_read;
  Stream::WriteCallback   o_write;
  Stream::ConnectCallback o_connect;

  Botan::RandomNumberGenerator& m_rng;
  Botan::Credentials_Manager&   m_creds;
  Botan::TLS::Strict_Policy     m_policy;
  Botan::TLS::Session_Manager_In_Memory m_session_manager;

  Botan::TLS::Server m_tls;
};

} // tls
} // net

#endif
