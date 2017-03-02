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

#include <service>
#include <net/http/client.hpp>
#include <net/http/server.hpp>
#include <net/inet4>
#include <info>
#include <timers>

std::unique_ptr<http::Client> client_;
std::unique_ptr<http::Server> server_;

void Service::start(const std::string&)
{

}

void Service::ready()
{
  auto& inet = net::Inet4::stack<0>(); // Inet4<VirtioNet>::stack<0>();
  inet.network_config(
    {  10,  0,  0, 46 },  // IP
    {  255,255,255, 0 },  // Netmask
    {  10,  0,  0,  1 },  // Gateway
    {  8,  8,  8,  8 }   // DNS
  );

  using namespace http;

  INFO("Server", "Testing server");

  server_ = std::make_unique<Server>(inet.tcp());

  server_->on_request([] (Request_ptr req, Response_writer_ptr writer)
  {
    printf("Received request:\n%s\n", req->to_string().c_str());
    // set content type
    writer->header().set_field(header::Content_Type, "text/plain");
    // write body
    writer->write("Hello");
  });

  server_->listen(8080);


  client_ = std::make_unique<Client>(inet.tcp());
  client_->on_send([] (Request& req, Client::Options& opt, const Client::Host host)
  {
    printf("Sending request:\n%s\n", req.to_string().c_str());
  });

  INFO("Client", "Testing against local server");

  auto req = client_->create_request();

  req->set_uri(uri::URI{"/testing"});
  client_->send(std::move(req), {inet.gateway(), 9011},
  [] (Error err, Response_ptr res, Connection&)
  {
    if (err)
      printf("Error: %s \n", err.to_string().c_str());

    printf("Received body: %s\n", res->body());
    CHECKSERT(!err, "No error");
    CHECKSERT(res->body() == "/testing", "Received body: \"/testing\"");

    using namespace std::chrono; // zzz...
    Timers::oneshot(5s, [](auto) { printf("SUCCESS\n"); });
  });


  INFO("Client", "Testing against Acorn");

  const std::string acorn_url{"http://acorn2.unofficial.includeos.io/"};

  client_->get(acorn_url, {},
  [] (Error err, Response_ptr res, Connection&)
  {
    CHECK(!err, "Error: %s", err.to_string().c_str());
    CHECK(res != nullptr, "Received response");
    if(!err)
      printf("Response:\n%s\n", res->to_string().c_str());
      });

  using namespace std::chrono;
  client_->get(acorn_url + "api/dashboard/status", {},
  [] (Error err, Response_ptr res, Connection&)
  {
    CHECK(!err, "Error: %s", err.to_string().c_str());
    CHECK(res != nullptr, "Received response");
    if(!err)
      printf("Response:\n%s\n", res->to_string().c_str());
  }, { 3s });

}
