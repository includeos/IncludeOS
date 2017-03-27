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

void Service::start(const std::string&)
{
  auto& inet = net::Inet4::stack<0>();
  inet.network_config(
    { 10, 0, 0, 48 },       // IP
    { 255, 255, 255, 0 },   // Netmask
    { 10, 0, 0, 1 },        // Gateway
    {  8, 8, 8, 8 }         // DNS
  );
  printf("Service IP address is %s\n", inet.ip_addr().str().c_str());

  inet.resolve("google.com", [] (IP4::addr a, Error err) {
    printf("Resolve callback. Address: %s\n", a.to_string().c_str());

    if (err)
      printf("Error occurred: %s\n", err.what());
    else
      printf("No error occurred\n");
  });
}
