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
#include <memdisk>
#include <net/inet>
#include <net/interfaces>
#include <https>
static http::Server* server = nullptr;
extern http::Response_ptr handle_request(const http::Request&);

void Service::start()
{
  extern void create_network_device(int N, const char* route, const char* ip);
  create_network_device(0, "10.0.0.0/24", "10.0.0.1");

  auto& inet = net::Interfaces::get(0);
  inet.network_config(
      {  10, 0,  0, 42 },  // IP
      { 255,255,255, 0 },  // Netmask
      {  10, 0,  0,  1 },  // Gateway
      {  10, 0,  0,  1 }); // DNS

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
  printf("Loaded certificates and keys\n");

  server = new http::S2N_server(
          ca_cert.to_string(), ca_key.to_string(), inet.tcp());

  server->on_request(
    [] (http::Request_ptr request,
        http::Response_writer_ptr response_writer) {
      response_writer->set_response(handle_request(*request));
      response_writer->write();
    });

  server->listen(443);
  printf("Using S2N for HTTPS transport\n");
}
