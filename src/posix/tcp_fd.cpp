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
#include <kernel/os.hpp>

using namespace net;

// return the "currently selected" networking stack
static Inet4& net_stack() {
  return Inet4::stack<> ();
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
  // connection
  if (this->cd) {
    int ret = cd->close();
    delete cd; cd = nullptr;
    return ret;
  }
  // listener
  if (this->ld) {
    int ret = ld->close();
    delete ld; ld = nullptr;
    return ret;
  }
  errno = EBADF;
  return -1;
}

int TCP_FD::connect(const struct sockaddr* address, socklen_t address_len)
{
  if (this->cd) {
    // if its straight-up connected, return that
    if (cd->conn->is_connected()) {
      errno = EISCONN;
      return -1;
    }
    // if the connection isn't closed, we can just assume its being used already
    if (!cd->conn->is_closed()) {
      errno = EALREADY;
      return -1;
    }
  }
  if (this->ld) {
    // already listening on port
    errno = EINVAL;
    return -1;
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
    OS::block();
  }
  // set connection whether good or bad
  if (outgoing->is_connected()) {
    // out with the old
    delete this->cd;
    // in with the new
    this->cd = new TCP_FD_Conn(outgoing);
    cd->set_default_read();
    return 0;
  }
  // failed to connect
  // TODO: try to distinguish the reason for connection failure
  errno = ECONNREFUSED;
  return -1;
}


ssize_t TCP_FD::send(const void* data, size_t len, int)
{
  if (!cd) {
    errno = EINVAL;
    return -1;
  }
  if (not cd->conn->is_connected()) {
    errno = ENOTCONN;
    return -1;
  }

  bool written = false;
  cd->conn->write(data, len,
  [&written] (bool) { written = true; });
  // sometimes we can just write and forget
  if (written) return len;
  while (!written) {
    OS::block();
  }
  return len;
}
ssize_t TCP_FD::recv(void* dest, size_t len, int)
{
  if (!cd) {
    errno = EINVAL;
    return -1;
  }
  // if the connection is closed or closing: read returns 0
  if (cd->conn->is_closed() || cd->conn->is_closing()) return 0;
  if (not cd->conn->is_connected()) {
    errno = ENOTCONN;
    return -1;
  }
  // read some bytes from readq
  int bytes = cd->readq.read((char*) dest, len);
  if (bytes) return bytes;
  
  bool done = false;
  // block and wait for more
  cd->conn->on_read(len,
  [&done, &bytes, dest] (auto buffer, size_t len) {
    // copy the data itself
    if (len)
        memcpy(dest, buffer.get(), len);
    // we are done
    done  = true;
    bytes = len;
  });

  // BLOCK HERE
  while (!done || !cd->conn->is_readable()) {
    OS::block();
  }
  // restore
  cd->set_default_read();
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

/// socket as connection

void TCP_FD_Conn::recv_to_ringbuffer(net::tcp::buffer_t buffer, size_t len)
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
void TCP_FD_Conn::set_default_read()
{
  // readq buffering (4kb at a time)
  conn->on_read(4096, {this, &TCP_FD_Conn::recv_to_ringbuffer});
}
int TCP_FD_Conn::close()
{
  conn->close();
  // wait for connection to close completely
  while (!conn->is_closed()) {
    OS::block();
  }
  return 0;
}

/// socket as listener

int TCP_FD_Listen::listen(int backlog)
{
  listener.on_connect(
  [this, backlog] (net::tcp::Connection_ptr conn)
  {
    // remove oldest if full
    if ((int) this->connq.size() >= backlog)
        this->connq.pop_back();
    // new connection
    this->connq.push_front(conn);
    /// if someone is blocking they should be leaving right about now
  });
  return 0;
}

int TCP_FD_Listen::close()
{
  //listener.close();
  return 0;
}
