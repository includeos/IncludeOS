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
#include <net/inet4>

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

void Service::start(const std::string&)
{
  auto& inet = net::Inet4::stack<0>();
  inet.network_config(
    { 10, 0, 0, 48 },       // IP
    { 255, 255, 255, 0 },   // Netmask
    { 10, 0, 0, 1 },        // Gateway
    {  8, 8, 8, 8 }         // DNS
  );

  const std::string google        = "google.com";
  const std::string github        = "github.com";
  const std::string guardian      = "theguardian.com";
  const std::string some_address  = "some_address_that_doesnt_exist.com";

  const IP4::addr stack_dns       = inet.dns_addr();
  const IP4::addr level3          = IP4::addr{4, 2, 2, 1};

  inet.resolve(google, [google, stack_dns] (IP4::addr res, const Error& err) {
    if (err) {
      print_error(google, stack_dns, err);
    }
    else {
      if (res != IP4::ADDR_ANY)
        print_success(google, stack_dns, res);
      else
        print_not_resolved(google);
    }
  });

  inet.resolve(github, [github, stack_dns] (IP4::addr res, const Error& err) {
    if (err) {
      print_error(github, stack_dns, err);
    }
    else {
      if (res != IP4::ADDR_ANY)
        print_success(github, stack_dns, res);
      else
        print_not_resolved(github);
    }
  });

  inet.resolve(guardian, level3, [guardian, level3] (IP4::addr res, const Error& err) {
    if (err) {
      print_error(guardian, level3, err);
    }
    else {
      if (res != IP4::ADDR_ANY)
        print_success(guardian, level3, res);
      else
        print_not_resolved(guardian);
    }
  });

  inet.resolve(some_address, [some_address, stack_dns] (IP4::addr res, const Error& err) {
    if (err) {
      print_error(some_address, stack_dns, err);
    }
    else {
      if (res != IP4::ADDR_ANY)
        print_success(some_address, stack_dns, res);
      else
        print_not_resolved(some_address);
    }
  });
}
