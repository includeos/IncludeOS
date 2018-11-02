// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2017 Oslo and Akershus University College of Applied Sciences
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
#include <net/inet>

using namespace net;

void print_error(const std::string& hostname, IP4::addr server, const Error& err) {
  printf("Error occurred when resolving IP address of %s with DNS server %s: %s\n", hostname.c_str(),
        server.to_string().c_str(), err.what());

  if (err.is_icmp()) {
    auto* pd = dynamic_cast<const ICMP_error*>(&err);
    printf("ICMP error type received when resolving %s: %s\n", hostname.c_str(), pd->icmp_type_str().c_str());
    printf("ICMP error code received when resolving %s: %s\n", hostname.c_str(), pd->icmp_code_str().c_str());
  }
}

void print_not_resolved(const std::string& hostname) {
  printf("%s couldn't be resolved\n", hostname.c_str());
}

void print_success(const std::string& hostname, IP4::addr server, IP4::addr res) {
  printf("Resolved IP address of %s with DNS server %s: %s\n", hostname.c_str(),
          server.to_string().c_str(), res.to_string().c_str());
}

struct Name_request
{
  std::string name;
  ip4::Addr   server;

  Name_request(std::string n)
    : name{std::move(n)} {}
  Name_request(std::string n, ip4::Addr serv)
    : name{std::move(n)}, server{serv} {}
};

static void do_test(net::Inet& inet, std::vector<Name_request>& reqs)
{
  for(auto& req : reqs)
  {
    if(req.server == 0)
      req.server = inet.dns_addr();

    inet.resolve(req.name, req.server,
      [name = req.name, server = req.server] (ip4::Addr res, const Error& err)
    {
      if (err) {
        print_error(name, server, err);
      }
      else {
        if (res != 0)
          print_success(name, server, res);
        else
          print_not_resolved(name);
      }
    });
  }
}

void Service::start(const std::string&)
{
  auto& inet = net::Inet::stack<0>();
  inet.network_config(
    { 10, 0, 0, 48 },       // IP
    { 255, 255, 255, 0 },   // Netmask
    { 10, 0, 0, 1 },        // Gateway
    {  1, 1, 1, 1 }         // DNS
  );

  const ip4::Addr level3{4, 2, 2, 1};
  const ip4::Addr google{8, 8, 8, 8};

  static std::vector<Name_request> requests {
    {"google.com", google},
    {"github.com", google},
    {"some_address_that_doesnt_exist.com"},
    {"theguardian.com", level3},
    {"www.facebook.com"},
    {"rs.dns-oarc.net"},
    {"reddit.com"},
    {"includeos.io"},
    {"includeos.org"},
    {"doubleclick.net"},
    {"google-analytics.com"},
    {"akamaihd.net"},
    {"googlesyndication.com"}
  };

  do_test(inet, requests);
}
