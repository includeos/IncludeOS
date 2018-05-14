// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015 Oslo and Akershus University College of Applied Sciences
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

#include <os>
#include <net/inet>
#include <timers>
#include <memdisk>
#include <net/openssl/init.hpp>
#include <net/openssl/tls_stream.hpp>
#include "liu.hpp"
static std::deque<net::Stream_ptr> strims;
static SSL_CTX* g_ctx = nullptr;

static void setup_callbacks(net::Stream& stream)
{
  stream.on_read(8192,
    [] (auto buf) {
      printf("Read: %.*s\n", (int) buf->size(), buf->data());
    });
  stream.on_close(
    [&stream] () {
      printf("Stream to %s closed\n", stream.to_string().c_str());
    });
}

static void save_state(liu::Storage& store, const liu::buffer_t*)
{
  printf("Save state called\n");
  for (auto& strim : strims)
  {
    auto* tls = (openssl::TLS_stream*) strim.get();
    auto* tcp = dynamic_cast<net::tcp::Stream*> (tls->transport());
    store.add_connection(1, tcp->tcp());

    store.add_tls_stream(2, *tls);
  }
  store.put_marker(3);
}
static void resume_state(liu::Restore& thing)
{
  printf("Resume state called\n");
  auto& inet = net::Super_stack::get(0);

  while (not thing.is_marker())
  {
    // restore tcp stream from storage
    auto tcp_stream = thing.as_tcp_stream(inet.tcp());
    thing.go_next();
    // create OpenSSL stream using TCP stream
    auto tls = thing.as_tls_stream(g_ctx, std::move(tcp_stream));
    thing.go_next();
    printf("Restored stream to %s\n", tls->to_string().c_str());
    // restore callbacks
    setup_callbacks(*tls);
    // store stream
    strims.push_back(std::move(tls));
  }
}

void Service::start()
{
  // Get the first IP stack
  auto& inet = net::Super_stack::get(0);

  // Print some useful netstats every 30 secs
  using namespace std::chrono;
  Timers::periodic(5s, 30s,
  [&inet] (uint32_t) {
    printf("<Service> TCP STATUS:\n%s\n", inet.tcp().status().c_str());
  });

  const char* tls_cert = "/test.pem";
  const char* tls_key  = "/test.key";
  const uint16_t tls_port = 12345;

  fs::memdisk().init_fs(
  [] (auto err, auto&) {
    assert(!err);
  });

  openssl::init();
  openssl::verify_rng();

  g_ctx = openssl::create_server(tls_cert, tls_key);
  printf("Done, listening on TCP port\n");

  inet.tcp().listen(tls_port,
    [] (net::tcp::Connection_ptr conn) {
      if (conn != nullptr)
      {
        auto* stream = new openssl::TLS_stream(
            g_ctx,
            std::make_unique<net::tcp::Stream>(conn)
        );
        stream->on_connect(
          [stream] (auto&) {
            printf("Connected to %s\n", stream->to_string().c_str());
            // -->
            strims.push_back(std::unique_ptr<openssl::TLS_stream> (stream));
            // <--
            setup_callbacks(*stream);
          });
          stream->on_close(
          [stream] () {
            delete stream;
          });
      }
    });

  setup_liveupdate_server(inet, 666, save_state);
  liu::LiveUpdate::resume("test", resume_state);
}

void Service::ready()
{
  printf("Service::ready\n");
}
