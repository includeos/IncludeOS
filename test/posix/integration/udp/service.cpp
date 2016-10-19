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

int main()
{
  struct sockaddr_in myaddr;

  /* bind to an arbitrary return address */
  /* because this is the client side, we don't care about the address */
  /* since no application will initiate communication here - it will */
  /* just send responses */
  /* INADDR_ANY is the IP address and 0 is the socket */
  /* htonl converts a long integer (e.g. address) to a network representation */
  /* htons converts a short integer (e.g. port) to a network representation */

  memset((char *)&myaddr, 0, sizeof(myaddr));
  myaddr.sin_family = AF_INET;
  myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  myaddr.sin_port = htons(0);

  int fd;
  if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    perror("cannot create socket");
    return 0;
  }
  if (bind(fd, (struct sockaddr *)&myaddr, sizeof(myaddr)) < 0) {
    perror("bind failed");
    return 0;
  }
  printf("SUCCESS");
}

void Service::start(const std::string&)
{
  main();
}
