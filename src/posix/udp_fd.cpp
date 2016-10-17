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

#include <udp_fd.hpp>
#include <kernel/irq_manager.hpp>
using namespace net;

// return the "currently selected" networking stack
static Inet4& net_stack() {
  return Inet4::stack<> ();
}

int UDP_FD::read(void*, size_t)
{
  return -1;
}
int UDP_FD::write(const void*, size_t)
{
  return -1;
}
int UDP_FD::close()
{
  return -1;
}
int UDP_FD::bind(const struct sockaddr *, socklen_t)
{
  return -1;
}
ssize_t UDP_FD::sendto(const void *, size_t, int, const struct sockaddr *, socklen_t)
{

  return 0;
}
ssize_t UDP_FD::recvfrom(void *__restrict__, size_t, int, struct sockaddr *__restrict__, socklen_t *__restrict__)
{

  return 0;
}
