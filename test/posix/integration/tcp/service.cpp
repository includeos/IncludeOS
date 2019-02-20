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
  inet.network_config({  10,  0,  0, 57 },   // IP
                      { 255, 255, 0,  0 },   // Netmask
                      {  10,  0,  0,  4 });  // Gateway

  INFO("TCP Socket", "bind(%u)", PORT);

  int fd = socket(AF_INET, SOCK_STREAM, 0);
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
  printf("err: %i %s res: %i\n", errno, strerror(errno), res);
  CHECKSERT(res < 0 && errno == EINVAL, "Fails when already bound (EINVAL)");

  res = bind(socket(AF_INET, SOCK_STREAM, 0), (struct sockaddr *)&myaddr, 1ul);
  CHECKSERT(res < 0 && errno == EINVAL, "Fails when address is invalid (EINVAL)");

  res = bind(socket(AF_INET, SOCK_STREAM, 0), (struct sockaddr *)&myaddr, sizeof(myaddr));
  CHECKSERT(res < 0 && errno == EADDRINUSE, "Port already bound (EADDRINUSE)");


  INFO("TCP Socket", "listen()");

  res = listen(fd, 5);
  CHECKSERT(res == 0, "Listen OK");

  INFO("TCP Socket", "accept()");
  int cfd = accept(fd, NULL, NULL);

  CHECKSERT(cfd > 0, "Accepted");

  unsigned char recvbuf[BUFSIZE]; // recv buffer

  INFO("TCP Socket", "recv()");
  res = recv(cfd, recvbuf, BUFSIZE, MSG_WAITALL);
  CHECKSERT(res > 0, "Received data MSG_WAITALL (%i bytes)", res);
  recvbuf[res] = 0;

  const char* rm_message = "POSIX is for hipsters";
  CHECKSERT(strcmp((char*)&recvbuf, rm_message) == 0, "Message is \"%s\"", recvbuf);

  memset(recvbuf, 0, BUFSIZE);
  res = recv(cfd, recvbuf, BUFSIZE, 0);
  recvbuf[res] = 0;
  CHECKSERT(res == 0, "No data received (closing)");

  res = shutdown(cfd, SHUT_RDWR);
  CHECKSERT(res < 0, "Shutdown on closed socket fails");
  res = close(cfd);
  CHECKSERT(res == 0, "Close socket");

  // We cant see if the buffer now is empty without blocking the test

  INFO("TCP Socket", "connect()");
  INFO2("Trigger TCP_recv");

  cfd = socket(AF_INET, SOCK_STREAM, 0);
  // destination address
  struct sockaddr_in destaddr;

  memset((char *)&destaddr, 0, sizeof(destaddr));
  destaddr.sin_family = AF_INET;
  destaddr.sin_addr.s_addr = htonl(inet.gateway().whole);
  destaddr.sin_port = htons(OUT_PORT);

  res = connect(cfd, (struct sockaddr *)&destaddr, sizeof(destaddr));
  CHECKSERT(res == 0, "Connect to remote address OK");

  INFO("TCP Socket", "send()");

  const char *my_message = "Only hipsters uses POSIX";
  res = send(cfd, my_message, strlen(my_message), 0);
  CHECKSERT(res > 0, "Send works when connected (verified by script)");

  res = close(cfd);
  CHECKSERT(res == 0, "Closed client connection");

  return 0;
}

void Service::start(const std::string&)
{
  main();
  Timers::oneshot(std::chrono::seconds(1), [](auto) {
    printf("SUCCESS\n");
  });
}
