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

#include <tcp_fd.hpp>
#include <kernel/irq_manager.hpp>
#include <netinet/in.h>
using namespace net;

// return the "currently selected" networking stack
static Inet4& net_stack() {
  return Inet4::stack<> ();
}

int TCP_FD::read(void*, size_t)
{
  return -1;
}
int TCP_FD::write(const void*, size_t)
{
  return -1;
}

int TCP_FD::connect(const struct sockaddr* saddr, socklen_t len)
{
  if (len != sizeof(sockaddr_in)) return -1;
  auto* inaddr = (sockaddr_in*) saddr;

  auto addr = ip4::Addr(inaddr->sin_addr);
  auto port = inaddr->sin_port;

  printf("[*] connecting to %s:%u...\n", addr.to_string().c_str(), port);
  auto outgoing = net_stack().tcp().connect({addr, port});
  // wait for connection state to change
  while (not (outgoing->is_connected() || outgoing->is_closing() || outgoing->is_closed())) {
    OS::halt();
    IRQ_manager::get().process_interrupts();
  }
  // set connection whether good or bad
  if (outgoing->is_connected()) {
    this->conn = outgoing;
    return 0;
  }
  this->conn = nullptr;
  return -1;
}

ssize_t TCP_FD::send(const void* data, size_t len, int)
{
  if (!conn) return -1;
  if (!conn->is_connected()) return -1;

  bool written = false;
  conn->write(data, len,
  [&written] (bool) { written = true; });
  // sometimes we can just write and forget
  if (written) return len;
  while (!written) {
    OS::halt();
    IRQ_manager::get().process_interrupts();
  }
  return len;
}

int TCP_FD::accept(struct sockaddr *__restrict__, socklen_t *__restrict__)
{
  return -1;
}
int TCP_FD::bind(const struct sockaddr *, socklen_t)
{
  return -1;
}
