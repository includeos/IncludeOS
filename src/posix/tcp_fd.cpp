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
int TCP_FD::close()
{
  return -1;
}

int TCP_FD::connect(const struct sockaddr* address, socklen_t address_len)
{
  if (this->conn) {
    // if its straight-up connected, return that
    if (this->conn->is_connected()) {
      errno = EISCONN;
      return -1;
    }
    // if the connection isn't closed, we can just assume its being used already
    if (!this->conn->is_closed()) {
      errno = EALREADY;
      return -1;
    }
  }

  if (address_len != sizeof(sockaddr_in)) {
    errno = EAFNOSUPPORT; // checkme?
    return -1;
  }
  auto* inaddr = (sockaddr_in*) address;

  auto addr = ip4::Addr(inaddr->sin_addr);
  auto port = inaddr->sin_port;

  printf("[*] connecting to %s:%u...\n", addr.to_string().c_str(), port);
  auto outgoing = net_stack().tcp().connect({addr, port});
  // O_NONBLOCK is set for the file descriptor for the socket and the connection
  // cannot be immediately established; the connection shall be established asynchronously.
  if (this->non_blocking) {
    errno = EINPROGRESS;
    return -1;
  }

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
