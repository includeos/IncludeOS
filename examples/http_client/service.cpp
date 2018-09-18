// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2016 Oslo and Akershus University College of Applied Sciences
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

#include <memdisk>
#include <net/openssl/init.hpp>

static SSL_CTX* init_ssl_context()
{
  auto& disk = fs::memdisk();
  disk.init_fs([] (fs::error_t err, auto& fs) {
    assert(!err);

    err = fs.print_subtree("/certs");
    assert(err == fs::no_error && "Need certificate bundle folder present");
  });

  auto ents = disk.fs().ls("/certs");
  // initialize client context
  openssl::init();
  return openssl::create_client(ents, true);
}

#include <service>
#include <net/http/client.hpp>
using namespace std::literals::string_literals;

static void begin_http(net::Inet& inet)
{
  using namespace http;

  static Basic_client basic{inet.tcp()};

  const auto url{"http://www.google.com"s};
  INFO("HTTP", "GET %s", url.c_str());

  basic.get(url, {}, [url](Error err, Response_ptr res, Connection&)
  {
    if(not err) {
      printf("\n%s - Got Response!\n%s\n", url.c_str(), res->to_string().c_str());
    }
    else {
      printf("\n%s - No response: %s\n", url.c_str(), err.to_string().c_str());
      printf("Make sure the virtual machine can reach internet.\n");
    }
  });

  auto* ctx = init_ssl_context();
  assert(ctx != nullptr);

  static Client client{inet.tcp(), ctx};

  const auto url_sec{"https://www.google.com"s};
  INFO("HTTPS", "(Secure) GET %s", url_sec.c_str());

  client.get("https://www.google.com", {}, [url = url_sec](Error err, Response_ptr res, Connection&)
  {
    if(not err) {
      printf("\n%s - Got Response!\n%s\n", url.c_str(), res->to_string().c_str());
    }
    else {
      printf("\n%s - No response: %s\n", url.c_str(), err.to_string().c_str());
      printf("Make sure the virtual machine can reach internet.\n");
    }
  });
}

void Service::start()
{
  auto& inet = net::Super_stack::get(0);

  inet.on_config(
    [] (auto& inet) {
      begin_http(inet);
    });
}
