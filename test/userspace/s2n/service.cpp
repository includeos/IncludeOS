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

// transport streams used when testing
#include "../fuzz/fuzzy_stream.hpp"
static fuzzy::Stream* ossl_fuzz_ptr = nullptr;
static fuzzy::Stream* s2n_fuzz_ptr = nullptr;

inline fuzzy::Stream_ptr create_stream(fuzzy::Stream** dest)
{
  return std::make_unique<fuzzy::Stream> (net::Socket{}, net::Socket{},
    [dest] (net::Stream::buffer_t buffer) -> void {
      (*dest)->give_payload(std::move(buffer));
    }, true);
}

static void do_test_serializing_tls(int index);
static void do_test_send_data();
static void do_test_completed();
static bool are_all_streams_at_stage(int stage);
static bool are_all_streams_atleast_stage(int stage);
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

    // serialize and deserialize TLS after connected
    do_test_serializing_tls(this->index);

    if (are_all_streams_at_stage(4))
    {
      printf("Now resending test data\n");
      // perform some writes at stage 4
      do_test_send_data();
    }

    // serialize and deserialize TLS again
    do_test_serializing_tls(this->index);

    if (are_all_streams_at_stage(NUM_STAGES)) {
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
bool are_all_streams_atleast_stage(int stage)
{
  return server_test.test_stage >= stage &&
         client_test.test_stage >= stage;
}
static void do_trash_memory()
{
  for (int i = 0; i < 1000; i++)
  {
    std::vector<char*> allocations;
    for (int i = 0; i < 1000; i++) {
      const size_t size = rand() % 0x1000;
      allocations.push_back(new char[size]);
      std::memset(allocations.back(), 0x0, size);
    }
    for (auto* alloc : allocations) {
      std::free(alloc);
    }
    allocations.clear();
  }
}
void do_test_serializing_tls(int index)
{
  char sbuffer[128*1024]; // server buffer
  char cbuffer[128*1024]; // client buffer
  printf(">>> Performing serialization / deserialization\n");
  // 1. serialize TLS, destroy streams
  const size_t sbytes =
      server_test.stream->serialize_to(sbuffer, sizeof(sbuffer));
  assert(sbytes > 0 && "Its only failed if it returned zero");
  const size_t cbytes =
      client_test.stream->serialize_to(cbuffer, sizeof(cbuffer));
  assert(cbytes > 0 && "Its only failed if it returned zero");

  // 2. deserialize TLS, create new streams
  //printf("Now deserializing TLS state\n");

  // 2.1: create new transport streams
  auto server_side = create_stream(&ossl_fuzz_ptr);
  s2n_fuzz_ptr = server_side.get();
  auto client_side = create_stream(&s2n_fuzz_ptr);
  ossl_fuzz_ptr = client_side.get();

  // 2.2: deserialize TLS config/context
  s2n::serial_free_config();
  do_trash_memory();
  s2n::serial_create_config();

  // 2.3: deserialize TLS streams
  // 2.3.1:
  auto dstream = s2n::TLS_stream::deserialize_from(
                  s2n::serial_get_config(),
                  std::move(server_side),
                  false,
                  sbuffer, sbytes
                );
  assert(dstream != nullptr && "Deserialization must return stream");
  server_test.stream = dstream.release();

  dstream = s2n::TLS_stream::deserialize_from(
                  s2n::serial_get_config(),
                  std::move(client_side),
                  false,
                  cbuffer, cbytes
                );
  assert(dstream != nullptr && "Deserialization must return stream");
  client_test.stream = dstream.release();

  // 3. set all delegates again
  server_test.setup_callbacks();
  client_test.setup_callbacks();
}
void do_test_send_data()
{
  server_test.send_data();
  client_test.send_data();
}
void do_test_completed()
{
  printf("SUCCESS\n");
  s2n::serial_free_config();
  os::shutdown();
}

void Service::start()
{
  printf("Service::start()\n");
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

  // initialize S2N and store the certificate/key pair
  printf("*** Initializing S2N\n");
  s2n::serial_test(ca_cert.to_string(), ca_key.to_string());
  printf("*** Create S2N configuration\n");
  s2n::serial_create_config();

  printf("*** Create fuzzy S2N streams\n");
  // server fuzzy stream
  auto server_side = create_stream(&ossl_fuzz_ptr);
  s2n_fuzz_ptr = server_side.get();
  printf("*** - 1. server-side created\n");
  // client fuzzy stream
  auto client_side = create_stream(&s2n_fuzz_ptr);
  ossl_fuzz_ptr = client_side.get();
  printf("*** - 2. client-side created\n");

  server_test.index = 0;
  server_test.stream =
    new s2n::TLS_stream(s2n::serial_get_config(), std::move(server_side), false);
  client_test.index = 1;
  client_test.stream =
    new s2n::TLS_stream(s2n::serial_get_config(), std::move(client_side), true);

  server_test.setup_callbacks();
  client_test.setup_callbacks();
  printf("* TLS streams created!\n");

  // try serializing and deserializing just after creation
  printf("*** Starting test...\n");
  do_test_serializing_tls(0);
}
