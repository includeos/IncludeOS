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

using namespace net;

// return the "currently selected" networking stack
static Inet4& net_stack() {
  return Inet4::stack<> ();
}

void TCP_FD::recv_to_ringbuffer(net::tcp::buffer_t buffer, size_t len)
{
  if (readq.free_space() < (int) len) {
    // make room for data
    int needed = len - readq.free_space();
    int discarded = readq.discard(needed);
    assert(discarded == needed);
  }
  // add data to ringbuffer
  int written = readq.write(buffer.get(), len);
  assert(written = len);
}
void TCP_FD::set_default_read()
{
  // readq buffering (4kb at a time)
  conn->on_read(4096, {this, &TCP_FD::recv_to_ringbuffer});
}

int TCP_FD::read(void* data, size_t len)
{
  return recv(data, len, 0);
}
int TCP_FD::write(const void* data, size_t len)
{
  return send(data, len, 0);
}

int TCP_FD::close()
{
  if (this->conn) {
    this->conn->close();
    // wait for connection to close completely
    while (!conn->is_closed()) {
      OS::halt();
      IRQ_manager::get().process_interrupts();
    }
    // free connection
    this->conn = nullptr;
    return 0;
  }
  errno = EBADF;
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

  auto addr = ip4::Addr(inaddr->sin_addr.s_addr);
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
    set_default_read();
    return 0;
  }
  this->conn = nullptr;
  return -1;
}


ssize_t TCP_FD::send(const void* data, size_t len, int)
{
  if (!conn) {
    errno = EBADF;
    return -1;
  }
  if (!conn->is_connected()) {
    //errno = ?
    return -1;
  }

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
ssize_t TCP_FD::recv(void* dest, size_t len, int)
{
  if (!conn) {
    errno = EBADF;
    return -1;
  }
  // if the connection is closed or closing: read returns 0
  if (conn->is_closed() || conn->is_closing()) return 0;
  if (not conn->is_connected()) {
    //errno = ?
    return -1;
  }
  // read some bytes from readq
  int bytes = readq.read((char*) dest, len);
  if (bytes) return bytes;
  
  bool done = false;
  // block and wait for more
  conn->on_read(len,
  [&done, &bytes, dest] (auto buffer, size_t len) {
    // copy the data itself
    if (len)
        memcpy(dest, buffer.get(), len);
    // we are done
    done  = true;
    bytes = len;
  });

  // BLOCK HERE
  while (!done || !conn->is_readable()) {
    OS::halt();
    IRQ_manager::get().process_interrupts();
  }
  // restore
  set_default_read();
  return bytes;
}

int TCP_FD::accept(struct sockaddr *__restrict__, socklen_t *__restrict__)
{
  return -1;
}
int TCP_FD::bind(const struct sockaddr *, socklen_t)
{
  return -1;
}
