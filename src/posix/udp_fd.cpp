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

// return the "currently selected" networking stack
static net::Inet4& net_stack() {
  return net::Inet4::stack<> ();
}

void UDP_FD::recv_to_buffer(net::UDPSocket::addr_t, net::UDPSocket::port_t, const char* buffer, size_t len)
{
  // buffer somehow
}
void UDP_FD::set_default_recv()
{
  assert(this->sock != nullptr && "Default recv called on nullptr");
  this->sock->on_read({this, &UDP_FD::recv_to_buffer});
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
int UDP_FD::bind(const struct sockaddr* address, socklen_t len)
{
  // we can assume this has already been bound since there is a pointer
  if(UNLIKELY(this->sock != nullptr)) {
    errno = EINVAL;
    return -1;
  }
  // The specified address is not a valid address for the address family of the specified socket.
  if(UNLIKELY(len != sizeof(struct sockaddr_in))) {
    errno = EAFNOSUPPORT;
    return -1;
  }
  // Bind
  const auto port = ((sockaddr_in*)address)->sin_port;
  auto& udp = net_stack().udp();
  try {
    this->sock = (port) ? &udp.bind(ntohs(port)) : &udp.bind();
    return 0;
  } catch(const net::UDP::Port_in_use_exception&) {
    errno = EADDRINUSE;
    return -1;
  }
}
ssize_t UDP_FD::sendto(const void* message, size_t len, int flags,
  const struct sockaddr* dest_addr, socklen_t dest_len)
{
  // The specified address is not a valid address for the address family of the specified socket.
  if(UNLIKELY(dest_len != sizeof(struct sockaddr_in))) {
    errno = EINVAL;
    return -1;
  }
  // Bind a socket if we dont already have one
  if(this->sock == nullptr) {
    this->sock = &net_stack().udp().bind();
  }
  const auto& dest = *((sockaddr_in*)dest_addr);
  // If the socket protocol supports broadcast and the specified address is a broadcast address for the socket protocol,
  // sendto() shall fail if the SO_BROADCAST option is not set for the socket.
  if(!broadcast_ && dest.sin_addr.s_addr == ntohl(INADDR_BROADCAST)) {

    return -1;
  }

  // Sending
  bool written = false;
  this->sock->sendto(ntohl(dest.sin_addr.s_addr), ntohs(dest.sin_port), message, len,
    [&written]() { written = true; });
  while(!written) {
    OS::halt();
    IRQ_manager::get().process_interrupts();
  }
  return len;
}
ssize_t UDP_FD::recvfrom(void *__restrict__ buffer, size_t len, int flags,
  struct sockaddr *__restrict__ address, socklen_t *__restrict__ address_len)
{
  if(UNLIKELY(this->sock == nullptr)) {
    errno = EINVAL; //
    return -1;
  }

  // Receiving
  int bytes = 0;
  bool done = false;
  this->sock->on_read(
    [&bytes, &done, buffer, len, address, address_len]
    (net::UDPSocket::addr_t addr, net::UDPSocket::port_t port, const char* data, size_t data_len)
    {
      bytes = std::min(len, data_len);
      memcpy(buffer, data, bytes);

      // TODO: If the actual length of the address is greater than the length of the supplied sockaddr structure,
      // the stored address shall be truncated.
      if(address != nullptr) {
        auto& sender = *((sockaddr_in*)address);
        sender.sin_family       = AF_INET;
        sender.sin_port         = htons(port);
        sender.sin_addr.s_addr  = htonl(addr.whole);
      }
      done = true;
    });

  while(!done) {
    OS::halt();
    IRQ_manager::get().process_interrupts();
  }

  return bytes;
}
