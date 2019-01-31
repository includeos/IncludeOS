// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2017-2018 IncludeOS AS, Oslo, Norway
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

#include <posix/tcp_fd.hpp>
#include <posix/fd_map.hpp>
#include <kernel/os.hpp>
#include <errno.h>
#include <netinet/in.h>
#include <net/interfaces.hpp>

//#define POSIX_STRACE
#ifdef POSIX_STRACE
#define PRINT(fmt, ...) printf(fmt, ##__VA_ARGS__)
#else
#define PRINT(fmt, ...) /* fmt */
#endif

using namespace net;

// return the "currently selected" networking stack
static auto& net_stack() {
  return Interfaces::get(0);
}

ssize_t TCP_FD::read(void* data, size_t len)
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
    PRINT("TCP: close(%s)\n", cd->to_string().c_str());
    int ret = cd->close();
    cd = nullptr;
    return ret;
  }
  // listener
  if (this->ld) {
    PRINT("TCP: close(%s)\n", ld->to_string().c_str());
    int ret = ld->close();
    delete ld; ld = nullptr;
    return ret;
  }
  return -EBADF;
}

long TCP_FD::connect(const struct sockaddr* address, socklen_t address_len)
{
  if (is_listener()) {
    PRINT("TCP: connect(%s) failed\n", ld->to_string().c_str());
    // already listening on port
    return -EINVAL;
  }
  if (this->cd) {
    PRINT("TCP: connect(%s) failed\n", cd->to_string().c_str());
    // if its straight-up connected, return that
    if (cd->conn->is_connected()) {
      return -EISCONN;
    }
    // if the connection isn't closed, we can just assume its being used already
    if (!cd->conn->is_closed()) {
      return -EALREADY;
    }
  }

  if (address_len != sizeof(sockaddr_in)) {
    return -EINVAL; // checkme?
  }
  auto* inaddr = (sockaddr_in*) address;

  auto addr = ip4::Addr(::ntohl(inaddr->sin_addr.s_addr));
  auto port = ::htons(inaddr->sin_port);

  PRINT("TCP: connect(%s:%u)\n", addr.to_string().c_str(), port);

  auto outgoing = net_stack().tcp().connect({addr, port});

  bool refused = false;
  outgoing->on_connect([&refused](auto conn) {
    refused = (conn == nullptr);
  });
  // O_NONBLOCK is set for the file descriptor for the socket and the connection
  // cannot be immediately established; the connection shall be established asynchronously.
  if (this->is_blocking() == false) {
    return -EINPROGRESS;
  }

  // wait for connection state to change
  while (not (outgoing->is_connected() or
              outgoing->is_closing() or
              outgoing->is_closed() or
              refused))
  {
    OS::block();
  }
  // set connection whether good or bad
  if (outgoing->is_connected()) {
    // out with the old, in with the new
    this->cd = std::make_unique<TCP_FD_Conn>(outgoing);
    cd->set_default_read();
    return 0;
  }
  // failed to connect
  // TODO: try to distinguish the reason for connection failure
  return -ECONNREFUSED;
}


ssize_t TCP_FD::send(const void* data, size_t len, int fmt)
{
  if (!cd) {
    return -EINVAL;
  }
  return cd->send(data, len, fmt);
}
ssize_t TCP_FD::sendto(const void* data, size_t len, int fmt,
                       const struct sockaddr* dest_addr, socklen_t dest_len)
{
  (void) dest_addr;
  (void) dest_len;
  return send(data, len, fmt);
}
ssize_t TCP_FD::recv(void* dest, size_t len, int flags)
{
  if (!cd) {
    return -EINVAL;
  }
  return cd->recv(dest, len, flags);
}

ssize_t TCP_FD::recvfrom(void* dest, size_t len, int flags, struct sockaddr*, socklen_t*)
{
  return recv(dest, len, flags);
}

long TCP_FD::accept(struct sockaddr *__restrict__ addr, socklen_t *__restrict__ addr_len)
{
  if (!ld) {
    return -EINVAL;
  }
  return ld->accept(addr, addr_len);
}
long TCP_FD::listen(int backlog)
{
  if (!ld) {
    return -EINVAL;
  }
  return ld->listen(backlog);
}
long TCP_FD::bind(const struct sockaddr *addr, socklen_t addrlen)
{
  //
  if (cd or ld) {
    return -EINVAL;
  }
  // verify socket address
  if (addrlen != sizeof(sockaddr_in)) {
    return -EINVAL;
  }
  auto* sin = (sockaddr_in*) addr;
  // verify its AF_INET
  if (sin->sin_family != AF_INET) {
    return -EAFNOSUPPORT;
  }
  // use sin_port for bind
  // its network order ... so swap that shit:
  uint16_t port = ::htons(sin->sin_port);
  // ignore IP address (FIXME?)
  /// TODO: verify that the IP is "local"
  try {
    auto& L = net_stack().tcp().listen(port);
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
    return -EADDRINUSE;
  }
}
int TCP_FD::shutdown(int mode)
{
  if (!cd) {
    return -EINVAL;
  }
  return cd->shutdown(mode);
}

/// socket default handler getters

TCP_FD::on_read_func TCP_FD::get_default_read_func()
{
  if (cd) {
    return {cd.get(), &TCP_FD_Conn::recv_to_ringbuffer};
  }
  if (ld) {
    return
    [/*this*/] (net::tcp::buffer_t) {
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
TCP_FD_Conn::TCP_FD_Conn(net::tcp::Connection_ptr c)
  : conn{std::move(c)}
{
  assert(conn != nullptr);
  set_default_read();

  conn->on_disconnect([this](auto self, auto reason) {
    this->recv_disc = true;
    (void) reason;
    //printf("dc: %s - %s\n", reason.to_string().c_str(), self->to_string().c_str());
    // do nothing, avoid close
    // net::tcp::Connection::Disconnect::CLOSING
    if(not self->is_connected())
      self->close();
  });
}

void TCP_FD_Conn::recv_to_ringbuffer(net::tcp::buffer_t buffer)
{
  if (readq.free_space() < (ssize_t) buffer->size()) {
    // make room for data
    int needed = buffer->size() - readq.free_space();
    int discarded = readq.discard(needed);
    assert(discarded == needed);
  }
  // add data to ringbuffer
  int written = readq.write(buffer->data(), buffer->size());
  assert(written == (ssize_t) buffer->size());
}
void TCP_FD_Conn::set_default_read()
{
  // readq buffering (16kb at a time)
  conn->on_read(16438, {this, &TCP_FD_Conn::recv_to_ringbuffer});
}
ssize_t TCP_FD_Conn::send(const void* data, size_t len, int)
{
  if (not conn->is_connected()) {
    return -ENOTCONN;
  }

  bool written = false;
  conn->on_write([&written] (bool) { written = true; }); // temp

  conn->write(data, len);

  // sometimes we can just write and forget
  if (written) return len;
  while (!written) {
    OS::block();
  }

  conn->on_write(nullptr); // temp
  return len;
}
ssize_t TCP_FD_Conn::recv(void* dest, size_t len, int)
{
  // read some bytes from readq
  int bytes = readq.read((char*) dest, len);
  if (bytes) return bytes;

  if(this->recv_disc)
    return 0;

  bool done = false;
  bytes = 0;

  // block and wait for more
  conn->on_read(len,
  [&done, &bytes, dest] (auto buffer) {
    // copy the data itself
    if (buffer->size() > 0)
        memcpy(dest, buffer->data(), buffer->size());
    // we are done
    done  = true;
    bytes = buffer->size();
  });

  // BLOCK HERE:
  // 1. if we havent read the data we asked for
  // 2. or we aren't readable but not closed (not 100% sure here hehe..)
  while (!done || (!conn->is_readable() and !conn->is_closed())) {
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
  if (conn->is_closed()) {
    return -ENOTCONN;
  }
  switch (mode) {
  case SHUT_RDWR:
    conn->close();
    return 0;
  case SHUT_RD:
    printf("Ignoring shutdown(SHUT_RD)\n");
    return 0;
  case SHUT_WR:
    printf("Ignoring shutdown(SHUT_WR)\n");
    return 0;
  default:
    return -EINVAL;
  }
}

/// socket as listener

long TCP_FD_Listen::listen(int backlog)
{
  listener.on_connect(
  [this, backlog] (net::tcp::Connection_ptr conn)
  {
    if(UNLIKELY(not conn))
      return;

    // remove oldest if full
    if ((int) this->connq.size() >= backlog)
        this->connq.pop_back();
    // new connection
    this->connq.push_front(std::make_unique<TCP_FD_Conn>(conn));
    /// if someone is blocking they should be leaving right about now
  });
  return 0;
}
long TCP_FD_Listen::accept(struct sockaddr *__restrict__ addr, socklen_t *__restrict__ addr_len)
{
  // block until connection appears
  while (connq.empty()) {
    OS::block();
  }
  // retrieve connection from queue
  auto sock = std::move(connq.back());
  connq.pop_back();

  assert(sock != nullptr);
  // create connected TCP socket
  auto& fd = FD_map::_open<TCP_FD>();
  fd.cd = std::move(sock);
  // set address and length
  if(addr != nullptr and addr_len != nullptr)
  {
    auto& conn = fd.cd->conn;
    auto* sin = (sockaddr_in*) addr;
    sin->sin_family      = AF_INET;
    sin->sin_port        = conn->remote().port();
    sin->sin_addr.s_addr = conn->remote().address().v4().whole;
    *addr_len = sizeof(sockaddr_in);
  }
  // return socket
  return fd.get_id();
}
int TCP_FD_Listen::close()
{
  listener.close();
  return 0;
}
