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
#include <stdio.h>
#include <info>

#include <net/inet4>
using namespace net;
std::unique_ptr<Inet4<VirtioNet>> inet;

#include <vga>
ConsoleVGA vga;

void Service::start()
{
  INFO("VGA", "Running tests for VGA");
  
  OS::set_rsprint(
  [] (const char* data, size_t len)
  {
    vga.write(data, len);
  });
  printf("Hello there!\n");
  
	hw::Nic<VirtioNet>& eth0 = hw::Dev::eth<0,VirtioNet>();
  inet = std::make_unique<Inet4<VirtioNet>>(eth0);
  inet->network_config(
      {{ 10,0,0,42 }},     // IP
			{{ 255,255,255,0 }}, // Netmask
			{{ 10,0,0,1 }},      // Gateway
			{{ 8,8,8,8 }} );     // DNS
  
  INFO("VGA", "SUCCESS");
}
