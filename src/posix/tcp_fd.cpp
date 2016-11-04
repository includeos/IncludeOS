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
#include <fd_map.hpp>
#include <kernel/os.hpp>

using namespace net;

// return the "currently selected" networking stack
static auto& net_stack() {
  return Inet4::stack();
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
  if (is_listener()) {
    // already listening on port
    errno = EINVAL;
    return -1;
  }
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

  if (address_len != sizeof(sockaddr_in)) {
    errno = EINVAL; // checkme?
    return -1;
  }
  auto* inaddr = (sockaddr_in*) address;

  auto addr = ip4::Addr(inaddr->sin_addr.s_addr);
  auto port = ::htons(inaddr->sin_port);

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


ssize_t TCP_FD::send(const void* data, size_t len, int fmt)
{
  if (!cd) {
    errno = EINVAL;
    return -1;
  }
  return cd->send(data, len, fmt);
}
ssize_t TCP_FD::recv(void* dest, size_t len, int flags)
{
  if (!cd) {
    errno = EINVAL;
    return -1;
  }
  return cd->recv(dest, len, flags);
}

int TCP_FD::accept(struct sockaddr *__restrict__ addr, socklen_t *__restrict__ addr_len)
{
  if (!ld) {
    errno = EINVAL;
    return -1;
  }
  return ld->accept(addr, addr_len);
}
int TCP_FD::listen(int backlog)
{
  if (!ld) {
    errno = EINVAL;
    return -1;
  }
  return ld->listen(backlog);
}
int TCP_FD::bind(const struct sockaddr *addr, socklen_t addrlen)
{
  //
  if (cd) {
    errno = EINVAL;
    return -1;
  }
  // verify socket address
  if (addrlen != sizeof(sockaddr_in)) {
    errno = EINVAL;
    return -1;
  }
  auto* sin = (sockaddr_in*) addr;
  // verify its AF_INET
  if (sin->sin_family != AF_INET) {
    errno = EAFNOSUPPORT;
    return -1;
  }
  // use sin_port for bind
  // its network order ... so swap that shit:
  uint16_t port = ::htons(sin->sin_port);
  // ignore IP address (FIXME?)
  /// verify that the IP is "local"
  try {
    auto& L = net_stack().tcp().bind(port);
    // remove existing listener
    if (ld) {
      int ret = ld->close();
      if (ret < 0) return ret;
      delete ld;
    }
    // create new one
    ld = new TCP_FD_Listen(L);
    return 0;

  } catch (...) {
    errno = EADDRINUSE;
    return -1;
  }
}
int TCP_FD::shutdown(int mode)
{
  if (!cd) {
    errno = EINVAL;
    return -1;
  }
  return cd->shutdown(mode);
}

/// socket default handler getters

TCP_FD::on_read_func TCP_FD::get_default_read_func()
{
  if (cd) {
    return {cd, &TCP_FD_Conn::recv_to_ringbuffer};
  }
  if (ld) {
    return
    [this] (net::tcp::buffer_t, size_t) {
      // what to do here?
    };
  }
  throw std::runtime_error("Invalid socket");
}
TCP_FD::on_write_func TCP_FD::get_default_write_func()
{
  return [] {};
}
TCP_FD::on_except_func TCP_FD::get_default_except_func()
{
  return [] {};
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
ssize_t TCP_FD_Conn::send(const void* data, size_t len, int)
{
  if (not conn->is_connected()) {
    errno = ENOTCONN;
    return -1;
  }

  bool written = false;
  conn->write(data, len,
  [&written] (bool) { written = true; });
  // sometimes we can just write and forget
  if (written) return len;
  while (!written) {
    OS::block();
  }
  return len;
}
ssize_t TCP_FD_Conn::recv(void* dest, size_t len, int)
{
  // if the connection is closed or closing: read returns 0
  if (conn->is_closed() || conn->is_closing()) return 0;
  if (not conn->is_connected()) {
    errno = ENOTCONN;
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
    OS::block();
  }
  // restore
  this->set_default_read();
  return bytes;
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
int TCP_FD_Conn::shutdown(int mode)
{
  if (not conn->is_connected()) {
    errno = ENOTCONN;
    return -1;
  }
  switch (mode) {
  case SHUT_RDRW:
    conn->close();
    return 0;
  case SHUT_RD:
    printf("Ignoring shutdown(SHUT_RD)\n");
    return 0;
  case SHUT_RW:
    printf("Ignoring shutdown(SHUT_RW)\n");
    return 0;
  default:
    errno = EINVAL;
    return -1;
  }
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
int TCP_FD_Listen::accept(struct sockaddr *__restrict__ addr, socklen_t *__restrict__ addr_len)
{
  // block until connection appears
  while (connq.empty()) {
    OS::block();
  }
  // retrieve connection from queue
  auto sock = connq.back();
  connq.pop_back();
  // make sure socket is connected, as promised
  if (not sock->is_connected()) {
    errno = ENOTCONN;
    return -1;
  }
  // create connected TCP socket
  auto& fd = FD_map::_open<TCP_FD>();
  fd.cd = new TCP_FD_Conn(sock);
  // set address and length
  auto* sin = (sockaddr_in*) addr;
  sin->sin_family      = AF_INET;
  sin->sin_port        = sock->remote().port();
  sin->sin_addr.s_addr = sock->remote().address().whole;
  *addr_len = sizeof(sockaddr_in);
  // return socket
  return fd.get_id();
}
int TCP_FD_Listen::close()
{
  net_stack().tcp().unbind(listener.local().port());
  return 0;
}
