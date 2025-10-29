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
#include <netinet/in.h>
#include <info>
#include <cassert>
#include <errno.h>
#include <unistd.h>
#include <net/interfaces>

const uint16_t PORT = 1042;
const uint16_t OUT_PORT = 4242;
const uint16_t BUFSIZE = 2048;

int main()
{
  auto&& inet = net::Interfaces::get(0);
  inet.network_config({  10,  0,  0, 58 },   // IP
                      { 255, 255, 0,  0 },   // Netmask
                      {  10,  0,  0,  3 });  // Gateway

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
  CHECKSERT(res < 0 && errno == EINVAL, "Fails when already bound (EINVAL)");

  res = bind(socket(AF_INET, SOCK_DGRAM, 0), (struct sockaddr *)&myaddr, 1ul);
  CHECKSERT(res < 0 && errno == EINVAL, "Fails when address is invalid (EINVAL)");

  res = bind(socket(AF_INET, SOCK_DGRAM, 0), (struct sockaddr *)&myaddr, sizeof(myaddr));
  CHECKSERT(res < 0 && errno == EADDRINUSE, "Port already bound (EADDRINUSE)");


  INFO("UDP Socket", "recvfrom()");

  /* remote address */
  struct sockaddr_in remaddr;
  socklen_t rem_addrlen = sizeof(remaddr); // size of remaddr
  unsigned char recvbuf[BUFSIZE]; // recv buffer

  res = recvfrom(fd, recvbuf, BUFSIZE, MSG_PEEK, (struct sockaddr *)&remaddr, &rem_addrlen);
  CHECKSERT(res > 0, "Received data w/ MSG_PEEK (%i bytes)", res);
  recvbuf[res] = 0;

  const char* rm_message = "POSIX is for hipsters";
  CHECKSERT(strcmp((char*)&recvbuf, rm_message) == 0, "Message is \"%s\"", recvbuf);

  CHECKSERT(remaddr.sin_addr.s_addr == htonl(inet.gateway().whole),
    "Received from address %s", inet.gateway().to_string().c_str());

  memset(recvbuf, 0, BUFSIZE);
  res = recvfrom(fd, recvbuf, BUFSIZE, 0, (struct sockaddr *)&remaddr, &rem_addrlen);
  recvbuf[res] = 0;
  CHECKSERT(res == (int)strlen(rm_message) && strcmp((char*)&recvbuf, rm_message) == 0,
    "Reading again returns the same message");

  // We cant see if the buffer now is empty without blocking the test

  INFO("UDP Socket", "sendto()");

  /* destination address */
  struct sockaddr_in destaddr;

  memset((char *)&destaddr, 0, sizeof(destaddr));
  destaddr.sin_family = AF_INET;
  destaddr.sin_addr.s_addr = htonl(inet.gateway().whole);
  destaddr.sin_port = htons(OUT_PORT);

  const char *my_message = "Only hipsters uses POSIX";
  res = sendto(socket(AF_INET, SOCK_DGRAM, 0), my_message, strlen(my_message), 0, (struct sockaddr *)&destaddr, sizeof(destaddr));
  CHECKSERT(res > 0, "Message was sent from NEW socket to %s:%u (verified by script)",
    inet.gateway().to_string().c_str(), OUT_PORT);

  INFO2("sendto() called");


  INFO("UDP Socket", "send() and connect()");
  int fd_send_connect = socket(AF_INET, SOCK_DGRAM, 0);

  res = send(fd_send_connect, my_message, strlen(my_message), 0);
  CHECKSERT(res < 0 && errno == EDESTADDRREQ, "Fails when not connected (EDESTADDRREQ)");

  res = connect(fd_send_connect, (struct sockaddr *)&destaddr, sizeof(destaddr));
  CHECKSERT(res == 0, "Connect to remote address OK");

  res = send(fd_send_connect, my_message, strlen(my_message), 0);
  CHECKSERT(res > 0, "Send works when connected (verified by script)");

  INFO2("sendto() called");

  INFO("UDP Socket", "reading from buffer with recv()");
  int i = 0;
  for(; i < 5; ++i) {
    res = recv(fd, recvbuf, BUFSIZE, 0);
    CHECKSERT(res > 0, "Received \"%.*s\"", res, recvbuf);
  }

  CHECKSERT(i == 5, "Received 5 messages");

  INFO("UDP Socket", "close()");

  res = close(fd);
  CHECKSERT(res == 0, "Close returns OK");
  CHECKSERT(not inet.udp().is_bound(PORT), "Underlying OS socket is closed");

  return 0;
}

void Service::start(const std::string&)
{
  main();
  printf("SUCCESS\n");
}
