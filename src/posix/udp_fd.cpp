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

#include <posix/udp_fd.hpp>
#include <os.hpp> // os::block()
#include <errno.h>
#include <net/interfaces.hpp>

//#define POSIX_STRACE 1
#ifdef POSIX_STRACE
#define PRINT(fmt, ...) printf(fmt, ##__VA_ARGS__)
#else
#define PRINT(fmt, ...) /* fmt */
#endif

// return the "currently selected" networking stack
static net::Inet& net_stack() {
  return net::Interfaces::get(0);
}

size_t UDP_FD::max_buffer_msgs() const
{
  return (rcvbuf_ / net_stack().udp().max_datagram_size());
}

void UDP_FD::recv_to_buffer(net::udp::addr_t addr,
  net::udp::port_t port, const char* buf, size_t len)
{
  // only recv to buffer if not full
  if(buffer_.size() < max_buffer_msgs()) {
    // copy data into to-be Message buffer
    auto buff = net::tcp::construct_buffer(buf, buf + len);
    // emplace the message in buffer
    buffer_.emplace_back(htonl(addr.v4().whole), htons(port), std::move(buff));
  }
}

int UDP_FD::read_from_buffer(void* buffer, size_t len, int flags,
  struct sockaddr* address, socklen_t* address_len)
{
  assert(!buffer_.empty() && "Trying to read from empty buffer");

  auto& msg = buffer_.front();
  auto& mbuf = msg.buffer;

  int bytes = std::min(len, mbuf->size());
  memcpy(buffer, mbuf->data(), bytes);

  if(address != nullptr) {
    memcpy(address, &msg.src, std::min(*address_len, (uint32_t) sizeof(struct sockaddr_in)));
    *address_len = sizeof(struct sockaddr_in);
  }

  if(!(flags & MSG_PEEK))
    buffer_.pop_front();

  return bytes;
}
void UDP_FD::set_default_recv()
{
  assert(this->sock != nullptr && "Default recv called on nullptr");
  this->sock->on_read({this, &UDP_FD::recv_to_buffer});
}
UDP_FD::~UDP_FD()
{
  // shutdown underlying socket, makes sure no callbacks are called on dead fd
  if(this->sock)
    sock->close();
}
ssize_t UDP_FD::read(void* buffer, size_t len)
{
  return recv(buffer, len, 0);
}
int UDP_FD::write(const void* buffer, size_t len)
{
  return send(buffer, len, 0);
}
int UDP_FD::close()
{
  return 0;
}
long UDP_FD::bind(const struct sockaddr* address, socklen_t len)
{
  // we can assume this has already been bound since there is a pointer
  if(UNLIKELY(this->sock != nullptr)) {
    return -EINVAL;
  }
  // invalid address
  if(UNLIKELY(len != sizeof(struct sockaddr_in))) {
    return -EINVAL;
  }
  // Bind
  const auto port = ((sockaddr_in*)address)->sin_port;
  auto& udp = net_stack().udp();
  try
  {
    this->sock = (port) ? &udp.bind(ntohs(port)) : &udp.bind();
    set_default_recv();
    PRINT("UDP: bind(%s)\n", sock->local().to_string().c_str());
    return 0;
  }
  catch(const net::UDP::Port_in_use_exception&)
  {
    return -EADDRINUSE;
  }
}
long UDP_FD::connect(const struct sockaddr* address, socklen_t address_len)
{
  if (UNLIKELY(address_len < sizeof(struct sockaddr_in))) {
    return -EINVAL;
  }

  // If the socket has not already been bound to a local address,
  // connect() shall bind it to an address which is an unused local address.
  if(this->sock == nullptr) {
    this->sock = &net_stack().udp().bind();
    set_default_recv();
  }

  const auto& addr = *((sockaddr_in*)address);
  if (addr.sin_family == AF_UNSPEC)
  {
    peer_.sin_addr.s_addr = 0;
    peer_.sin_port        = 0;
  }
  else
  {
    peer_.sin_family = AF_INET;
    peer_.sin_addr   = addr.sin_addr;
    peer_.sin_port   = addr.sin_port;
  }
  PRINT("UDP: connect(%s:%u)\n",
        net::IP4::addr(peer_.sin_addr.s_addr).to_string().c_str(),
        htons(peer_.sin_port));

  return 0;
}

ssize_t UDP_FD::sendto(const void* message, size_t len, int,
  const struct sockaddr* dest_addr, socklen_t dest_len)
{
  if(not is_connected())
  {
    if(UNLIKELY((dest_addr == nullptr or dest_len == 0))) {
      return -EDESTADDRREQ;
    }
    // The specified address is not a valid address for the address family of the specified socket.
    else if(UNLIKELY(dest_len != sizeof(struct sockaddr_in))) {
      return -EAFNOSUPPORT;
    }
  }

  // Bind a socket if we dont already have one
  if(this->sock == nullptr) {
    this->sock = &net_stack().udp().bind();
    set_default_recv();
  }
  const auto& dest = (not is_connected()) ? *((sockaddr_in*)dest_addr) : peer_;
  // If the socket protocol supports broadcast and the specified address
  // is a broadcast address for the socket protocol,
  // sendto() shall fail if the SO_BROADCAST option is not set for the socket.
  if(!broadcast_ && dest.sin_addr.s_addr == INADDR_BROADCAST) { // Fix me
    return -EOPNOTSUPP;
  }

  // Sending
  bool written = false;
  this->sock->sendto(net::ip4::Addr{ntohl(dest.sin_addr.s_addr)}, ntohs(dest.sin_port), message, len,
    [&written]() { written = true; });

  while(!written)
    os::block();

  return len;
}
ssize_t UDP_FD::recv(void* buffer, size_t len, int flags)
{
  PRINT("UDP: recv(%lu, %x)\n", len, flags);
  return recvfrom(buffer, len, flags, nullptr, 0);
}
ssize_t UDP_FD::recvfrom(void *__restrict__ buffer, size_t len, int flags,
  struct sockaddr *__restrict__ address, socklen_t *__restrict__ address_len)
{
  if(UNLIKELY(this->sock == nullptr)) {
    errno = EINVAL; //
    return -1;
  }

  // Read from buffer if not empty
  if(!buffer_.empty())
  {
    return read_from_buffer(buffer, len, flags, address, address_len);
  }
  // Else make a blocking receive
  else
  {
    int bytes = 0;
    bool done = false;

    this->sock->on_read(net::udp::Socket::recvfrom_handler::make_packed(
    [&bytes, &done, this,
      buffer, len, flags, address, address_len]
    (net::udp::addr_t addr, net::udp::port_t port,
      const char* data, size_t data_len)
    {
      // if this already been called once while blocking, buffer
      if(done) {
        recv_to_buffer(addr, port, data, data_len);
        return;
      }

      bytes = std::min(len, data_len);
      memcpy(buffer, data, bytes);

      // TODO: If the actual length of the address is greater than the length of the supplied sockaddr structure,
      // the stored address shall be truncated.
      if(address != nullptr) {
        auto& sender = *((sockaddr_in*)address);
        sender.sin_family       = AF_INET;
        sender.sin_port         = htons(port);
        sender.sin_addr.s_addr  = htonl(addr.v4().whole);
        *address_len            = sizeof(struct sockaddr_in);
      }
      done = true;

      // Store in buffer if PEEK
      if(flags & MSG_PEEK)
        recv_to_buffer(addr, port, data, data_len);
    }));

    // Block until (any) data is read
    while(!done)
      os::block();

    set_default_recv();

    return bytes;
  }
}
int UDP_FD::getsockopt(int level, int option_name,
  void *option_value, socklen_t *option_len)
{
  PRINT("UDP: getsockopt(%d, %d)\n", level, option_name);
  if(level != SOL_SOCKET)
    return -1;

  switch(option_name)
  {
    case SO_ACCEPTCONN:
    {
      errno = ENOPROTOOPT;
      return -1;
    }
    case SO_BROADCAST:
    {
      if(*option_len < (int)sizeof(int))
      {
        errno = EINVAL;
        return -1;
      }

      *((int*)option_value) = broadcast_;
      *option_len = sizeof(broadcast_);
      return 0;
    }
    case SO_KEEPALIVE:
    {
      errno = ENOPROTOOPT;
      return -1;
    }
    case SO_RCVBUF:
    {
      if(*option_len < (int)sizeof(int))
      {
        errno = EINVAL;
        return -1;
      }

      *((int*)option_value) = rcvbuf_;
      *option_len = sizeof(rcvbuf_);
      return 0;
    }
    // Address can always be reused in IncludeOS
    case SO_REUSEADDR:
    {
      if(*option_len < (int)sizeof(int))
      {
        errno = EINVAL;
        return -1;
      }

      *((int*)option_value) = 1;
      *option_len = sizeof(int);
      return 0;
    }
    case SO_TYPE:
    {
      if(*option_len < (int)sizeof(int))
      {
        errno = EINVAL;
        return -1;
      }

      *((int*)option_value) = SOCK_DGRAM;
      *option_len = sizeof(int);
      return 0;
    }

    default:
      return -1;
  } // < switch(option_name)
}

int UDP_FD::setsockopt(int level, int option_name,
  const void *option_value, socklen_t option_len)
{
  PRINT("UDP: setsockopt(%d, %d, ... %d)\n", level, option_name, option_len);
  if(level != SOL_SOCKET)
    return -1;

  switch(option_name)
  {
    case SO_BROADCAST:
    {
      if(option_len < (int)sizeof(int))
        return -1;

      broadcast_ = *((int*)option_value);
      return 0;
    }
    case SO_KEEPALIVE:
    {
      errno = ENOPROTOOPT;
      return -1;
    }
    case SO_RCVBUF:
    {
      if(option_len < (int)sizeof(int))
        return -1;

      rcvbuf_ = *((int*)option_value);
      return 0;
    }
    // Address can always be reused in IncludeOS
    case SO_REUSEADDR:
    {
      return 0;
    }

    default:
      return -1;
  } // < switch(option_name)
}
