// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015 Oslo and Akershus University College of Applied Sciences
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

#include <sys/socket.h>
#include <net/inet4>
using namespace net;
using Connection_ptr = tcp::Connection_ptr;
extern Connection_ptr __blocking_connect(IP4::addr addr, uint16_t port);
extern void __blocking_write(Connection_ptr, const char* data, size_t len);
extern void __blocking_close(Connection_ptr);

struct DescBase
{
  sa_family_t dom;
  uint8_t     type;
};
struct TCPdesc : public DescBase
{
  TCPdesc() {}

  tcp::Connection_ptr conn = nullptr;
};
struct UDPdesc : public DescBase
{
  UDPdesc() {}

  UDPSocket* sock = nullptr;
};

struct Impl
{
  std::vector<void*>  fds;
  std::vector<size_t> free_fds;

  int new_socket();

  void*& get(int fd) {
    return fds[fd];
  }

  // we can always look at the base values for a socket
  DescBase& get_base(int fd) {
    return *(DescBase*) fds[fd];
  }

  int fd_count() const noexcept {
    return fds.size();
  }
};
// in case we want per-cpu networking stacks
static Impl impl;

int Impl::new_socket() {
  if (!free_fds.empty()) {
    auto sock = free_fds.back();
    free_fds.pop_back();
    return sock;
  }
  fds.emplace_back(nullptr);
  return fds.size()-1;
}

bool verify_address(uint8_t dom, socklen_t len)
{
  if (dom == AF_INET  && len ==  4) return true;
  if (dom == AF_INET6 && len == 16) return true;
  return false;
}

///////////////////////////////////////////////////////////////////////////////
/// POSIX function calls
///////////////////////////////////////////////////////////////////////////////

int socket(int domain, int type, int protocol)
{
  // disallow strange domains, like ALG
  if (domain < 0 || domain > AF_INET6) { errno = EAFNOSUPPORT; return -1; }
  // disallow RAW etc
  if (type < 0 || type > SOCK_DGRAM) { errno = EINVAL; return -1; }
  // we are purposefully ignoring the protocol argument
  if (protocol < 0) { errno = EPROTONOSUPPORT; return -1; }

  auto sock = impl.new_socket();
  // let's assume IPv4 for now
  if (type == SOCK_STREAM) {
    impl.get(sock) = (void*) new TCPdesc;
  }
  else {
    impl.get(sock) = (void*) new UDPdesc;
  }
  //
  auto& header = impl.get_base(sock);
  header.dom  = domain;
  header.type = type;

  return sock;
}

int connect(int socket, const struct sockaddr *address, socklen_t address_len)
{
  if (socket < 0 || socket >= impl.fd_count()) {
    errno = EINVAL;
    return -1;
  }
  auto& base = impl.get_base(socket);
  // for domain AF_INET the address length must be 4
  if (!verify_address(base.dom, address_len))
  /// determine if the socket supports connect()
  if (base.dom == AF_INET && base.type == SOCK_STREAM) {
    // its a IPv4 TCP socket, so...
    auto* tcp = (TCPdesc*) &base;
    // connect to the socket specified in address
    tcp->conn = __blocking_connect({"10.0.0.1"}, 6667);
    // its null if the connect failed
    return tcp->conn != nullptr;
  }
  errno = EINVAL;
  return -1;
}

ssize_t send(int socket, const void *message, size_t length, int)
{
  auto& base = impl.get_base(socket);
  /// determine if the socket supports connect()
  if (base.dom == AF_INET && base.type == SOCK_STREAM) {
    // its a IPv4 TCP socket, so...
    auto* tcp = (TCPdesc*) &base;
    if (tcp->conn->is_connected()) {
      __blocking_write(tcp->conn, (const char*) message, length);
      if (tcp->conn->is_connected()) return length;
      return 0;
    }
  }
  return -1;
}


int     listen(int socket, int backlog);
