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
#include <cstdio>
#include <sys/socket.h>
#include <info>
#include <cassert>
#include <errno.h>
#include <net/inet4>

const uint16_t PORT = 42;
const uint16_t OUT_PORT = 4242;

int main()
{
  auto&& inet = net::Inet4::ifconfig({  10,  0,  0, 45 },   // IP
                                     { 255, 255, 0,  0 },   // Netmask
                                     {  10,  0,  0,  1 });  // Gateway

  INFO("UDP Socket", "bind(%u)", PORT);

  int fd = socket(AF_INET, SOCK_DGRAM, 0);
  CHECKSERT(fd > 0, "Socket was created");

  /* local address */
  struct sockaddr_in myaddr;

  memset((char *)&myaddr, 0, sizeof(myaddr));
  myaddr.sin_family = AF_INET;
  myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  myaddr.sin_port = htons(PORT);

  int res = bind(fd, (struct sockaddr *)&myaddr, sizeof(myaddr));
  CHECKSERT(res == 0, "Socket was bound to port %u", PORT);

  res = bind(fd, (struct sockaddr *)&myaddr, sizeof(myaddr));
  CHECKSERT(res < 0 && errno == EINVAL, "EINVAL: Fails when already bound");

  res = bind(socket(AF_INET, SOCK_DGRAM, 0), (struct sockaddr *)&myaddr, 1ul);
  CHECKSERT(res < 0 && errno == EAFNOSUPPORT, "EAFNOSUPPORT: Fails when address is invalid");

  res = bind(socket(AF_INET, SOCK_DGRAM, 0), (struct sockaddr *)&myaddr, sizeof(myaddr));
  CHECKSERT(res < 0 && errno == EADDRINUSE, "EADDRINUSE: Port already bound");


  INFO("UDP Socket", "sendto()");

  /* destination address */
  struct sockaddr_in destaddr;

  memset((char *)&destaddr, 0, sizeof(destaddr));
  destaddr.sin_family = AF_INET;
  destaddr.sin_addr.s_addr = htonl(inet.router().whole);
  destaddr.sin_port = htons(OUT_PORT);

  const char *my_message = "Only hipsters uses POSIX";

  res = sendto(socket(AF_INET, SOCK_DGRAM, 0), my_message, strlen(my_message), 0, (struct sockaddr *)&destaddr, sizeof(destaddr));
  CHECKSERT(res > 0, "Message was sent from NEW socket to %s:%u (verified by script)",
    inet.router().to_string().c_str(), OUT_PORT);


  printf("SUCCESS\n");
  return 0;
}

void Service::start(const std::string&)
{
  main();
}
