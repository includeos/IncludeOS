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
#include "serial.hpp"
#include "../fuzz/fuzzy_stream.hpp"
static fuzzy::Stream* ossl_fuzz_ptr = nullptr;
static fuzzy::Stream* s2n_fuzz_ptr = nullptr;

static void do_test_serializing_tls(int index);
static void do_test_completed();
static bool are_all_streams_at_stage(int stage);
static void test_failure(const std::string& data) {
  printf("Received unexpected data: %s\n", data.c_str());
  printf("Length: %zu bytes\n", data.size());
  std::abort();
}
static std::string long_string(32000, '-');

struct Testing
{
  static const int NUM_STAGES = 7;
  int index = 0;
  int test_stage = 0;
  s2n::TLS_stream* stream = nullptr;
  std::string read_buffer = "";

  void send_data()
  {
    this->stream->write("Hello!");
    this->stream->write("Second write");
    this->stream->write(long_string);
  }
  void onread_function(net::Stream::buffer_t buffer)
  {
    assert(this->stream != nullptr && stream->is_connected());
    read_buffer += std::string(buffer->begin(), buffer->end());
    if (read_buffer == "Hello!") this->test_stage_advance();
    else if (read_buffer == "Second write") this->test_stage_advance();
    else if (read_buffer == long_string) this->test_stage_advance();
    // else: ... wait for more data
  }
  void connect_function(net::Stream& stream)
  {
    this->test_stage_advance();
    printf("TLS stream connected (%d / 2)\n", test_stage);
    this->send_data();
  }
  void test_stage_advance()
  {
    this->test_stage ++;
    this->read_buffer.clear();
    printf("[%d] Test stage: %d / %d\n", 
           this->index, this->test_stage, NUM_STAGES);
    if (are_all_streams_at_stage(4))
    {
      do_test_serializing_tls(this->index);
    }
    else if (are_all_streams_at_stage(NUM_STAGES)) {
      do_test_completed();
    }
  }
  void setup_callbacks()
  {
    stream->on_connect({this, &Testing::connect_function});
    stream->on_read(8192, {this, &Testing::onread_function});
  }
};
static struct Testing server_test;
static struct Testing client_test;

bool are_all_streams_at_stage(int stage)
{
  return server_test.test_stage == stage &&
         client_test.test_stage == stage;
}
void do_test_serializing_tls(int index)
{
  printf("Now serializing TLS state\n");
  // 1. serialize TLS, destroy streams
  // TODO: implement me
  // 2. deserialize TLS, create new streams
  // TODO: implement me
  
  printf("Now restarting test\n");
  // 3. set all delegates again
  server_test.setup_callbacks();
  client_test.setup_callbacks();
  // 4. send more data and wait for responses
  server_test.send_data();
  client_test.send_data();
}
void do_test_completed()
{
  printf("SUCCESS\n");
  s2n::serial_test_over();
  OS::shutdown();
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
  s2n::serial_test(ca_cert.to_string(), ca_key.to_string());
  
  // server stream
  auto server_side = std::make_unique<fuzzy::Stream> (net::Socket{}, net::Socket{},
    [] (net::Stream::buffer_t buffer) {
      ossl_fuzz_ptr->give_payload(std::move(buffer));
    }, true);
  s2n_fuzz_ptr = server_side.get();
  
  server_test.index = 0;
  server_test.stream =
    new s2n::TLS_stream(s2n::serial_get_config(), std::move(server_side), false);
  client_test.index = 1;
  client_test.stream =
    new s2n::TLS_stream(s2n::serial_get_config(), std::move(client_side), true);
  
  server_test.setup_callbacks();
  client_test.setup_callbacks();
}
