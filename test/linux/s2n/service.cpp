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
#include <memdisk>
#include <net/inet>
#include <net/interfaces>
#include <net/s2n/stream.hpp>
#include "../fuzz/fuzzy_stream.hpp"
static fuzzy::Stream* ossl_fuzz_ptr = nullptr;
static fuzzy::Stream* s2n_fuzz_ptr = nullptr;

static int test_stage = 0;
static const int NUM_STAGES = 4;

void test_stage_advance() {
  test_stage ++;
  printf("Test stage: %d / %d\n", test_stage, NUM_STAGES);
  if (test_stage == NUM_STAGES) {
    printf("SUCCESS\n");
    extern void s2n_serial_test_over();
    s2n_serial_test_over();
    OS::shutdown();
  }
}
void test_failure(const std::string& data) {
  printf("Received unexpected data: %s\n", data.c_str());
  std::abort();
}

void Service::start()
{
  fs::memdisk().init_fs(
  [] (fs::error_t err, fs::File_system&) {
    assert(!err);
  });

  auto& filesys = fs::memdisk().fs();
  auto ca_cert = filesys.read_file("/test.pem");
  assert(ca_cert.is_valid());
  auto ca_key  = filesys.read_file("/test.key");
  assert(ca_key.is_valid());
  auto srv_key = filesys.read_file("/server.key");
  assert(srv_key.is_valid());
  printf("*** Loaded certificates and keys\n");

  // client stream
  auto client_side = std::make_unique<fuzzy::Stream> (net::Socket{}, net::Socket{},
    [] (net::Stream::buffer_t buffer) {
      s2n_fuzz_ptr->give_payload(std::move(buffer));
    }, true);
  ossl_fuzz_ptr = client_side.get();

  // initialize S2N
  extern void        s2n_serial_test(const std::string&, const std::string&);
  extern s2n_config* s2n_serial_get_config();
  s2n_serial_test(ca_cert.to_string(), ca_key.to_string());
  
  // server stream
  auto server_side = std::make_unique<fuzzy::Stream> (net::Socket{}, net::Socket{},
    [] (net::Stream::buffer_t buffer) {
      ossl_fuzz_ptr->give_payload(std::move(buffer));
    }, true);
  s2n_fuzz_ptr = server_side.get();
  
  auto* server_stream =
    new s2n::TLS_stream(s2n_serial_get_config(), std::move(server_side), false);
  auto* client_stream =
    new s2n::TLS_stream(s2n_serial_get_config(), std::move(client_side), true);
  
  server_stream->on_connect([] (net::Stream& stream) {
    printf("TLS server stream connected to client!\n");
    stream.on_read(8192, [] (net::Stream::buffer_t buffer) {
      std::string data(buffer->begin(), buffer->end());
      if (data == "Hello!") test_stage_advance();
      else if (data == "Second write") test_stage_advance();
      else test_failure(data);
    });
    stream.write("Hello!");
    stream.write("Second write");
  });
  client_stream->on_connect([] (net::Stream& stream) {
    printf("TLS client stream connected to server!\n");
    stream.on_read(8192, [] (net::Stream::buffer_t buffer) {
      std::string data(buffer->begin(), buffer->end());
      if (data == "Hello!") test_stage_advance();
      else if (data == "Second write") test_stage_advance();
      else test_failure(data);
    });
    stream.write("Hello!");
    stream.write("Second write");
  });
}
