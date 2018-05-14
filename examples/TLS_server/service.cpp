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
#include <timers>
#include <https>
#define BENCHMARK_MODE
static const bool ENABLE_TLS    = true;
static const bool USE_BOTAN_TLS = false;

static http::Server* server = nullptr;
extern http::Response_ptr handle_request(const http::Request&);

void Service::start()
{
  fs::memdisk().init_fs(
  [] (auto err, auto&) {
    assert(!err);
  });
  // Auto-configured from config.json
  auto& inet = net::Super_stack::get<net::IP4>(0);

#ifndef BENCHMARK_MODE
  // Print some useful TCP stats every 30 secs
  Timers::periodic(5s, 30s,
  [&inet] (uint32_t) {
    printf("<Service> TCP STATUS:\n%s\n", inet.tcp().status().c_str());
  });
#endif

  if (USE_BOTAN_TLS) {
    auto& filesys = fs::memdisk().fs();
    auto ca_cert = filesys.stat("/test.pem");
    auto ca_key  = filesys.stat("/test.key");
    auto srv_key = filesys.stat("/server.key");

    server = new http::Botan_server(
          "blabla", ca_key, ca_cert, srv_key, inet.tcp());
    printf("Using Botan for HTTPS transport\n");
  }
  else {
    server = new http::OpenSSL_server(
            "/test.pem", "/test.key", inet.tcp());
    printf("Using OpenSSL for HTTPS transport\n");
  }

  server->on_request(
    [] (auto request, auto response_writer) {
      response_writer->set_response(handle_request(*request));
      response_writer->write();
    });

  // listen on default HTTPS port
  server->listen(443);
}

#ifdef BENCHMARK_MODE
#include <profile>
static void print_heap_info()
{
  const std::string heapinfo = HeapDiag::to_string();
  printf("%s\n", heapinfo.c_str());
  StackSampler::print(10);
}

void Service::ready()
{
  using namespace std::chrono;
  Timers::periodic(1s, [] (int) {
    print_heap_info();
  });

  StackSampler::begin();
}
#endif
