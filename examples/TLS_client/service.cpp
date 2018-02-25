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

#include <service>
#include <net/inet4>
#include <memdisk>
#include <https>
#include <net/openssl/init.hpp>
#include <net/openssl/tls_stream.hpp>

namespace acorn {
  extern void list_static_content(const fs::File_system&);
}

void Service::start()
{
  // Get the first IP stack
  // It should have configuration from config.json
  auto& inet = net::Super_stack::get<net::IP4>(0);

  fs::memdisk().init_fs(
  [] (auto err, auto& fs) {
    assert(!err);
    //acorn::list_static_content(fs);
    (void) fs;
  });

  // initialize client context
  openssl::init();
  auto* ctx = openssl::create_client("/ca/");

  printf("Connecting to https://www.google.com...");
  inet.tcp().connect(
    {{"173.194.221.105"}, 443},
    [ctx] (auto conn) {
      if (conn == nullptr) {
        printf("Could not connect...\n");
        return;
      }
      printf("Connected to %s\n", conn->remote().to_string().c_str());
      auto tcp_stream = std::make_unique<net::tcp::Connection::Stream>(std::move(conn));

      auto* stream = new openssl::TLS_stream(ctx, std::move(tcp_stream), true);
      stream->on_connect(
        [] (auto& stream)
        {
          stream.write("GET /\r\n\r\n");

          stream.on_read(8192,
          [strim = &stream] (auto buf) {
            printf("on_read(%lu): %.*s\n",
                  buf->size(),
                  (int) buf->size(), buf->data());
            strim->write(buf);
          });
        });
      stream->on_close(
        [stream] {
          printf("Stream closed\n");
          delete stream;
        });
    });

}
