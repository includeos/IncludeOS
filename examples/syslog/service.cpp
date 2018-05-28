// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015-2016 Oslo and Akershus University College of Applied Sciences
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

/* For Posix */
#include <syslog.h>

/* For IncludeOS */
// #include <syslogd>

extern "C" int main();

void Service::start(const std::string&)
{
  // DHCP on interface 0
  auto& inet = net::Inet::ifconfig(10.0);
  // static IP in case DHCP fails
  net::Inet::ifconfig({  10,  0,  0, 45 },   // IP
                      { 255, 255, 0,  0 },    // Netmask
                      {  10,  0,  0,  1 },    // Gateway
                      {  10,  0,  0,  1} );   // DNS


  // For now we have to specify the remote syslogd IP
  // FIXME: instructions  here

  // Starts the python integration test:
  printf("Service IP address is %s\n", inet.ip_addr().str().c_str());

  /* ------------------------- POSIX syslog in IncludeOS ------------------------- */
  main();
}
