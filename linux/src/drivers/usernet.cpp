#include "usernet.hpp"

constexpr MAC::Addr UserNet::MAC_ADDRESS;

UserNet::UserNet()
  : Link(Link_protocol{{this, &UserNet::transmit}, mac()},
    256u, 2048 /* 256x half-page buffers */)
{

}

size_t UserNet::transmit_queue_available()
{
  return 128;
}

void UserNet::transmit(net::Packet_ptr packet)
{
  assert(transmit_forward_func);
  transmit_forward_func(std::move(packet));
}
void UserNet::feed(net::Packet_ptr packet)
{
  // wrap in packet, pass to Link-layer
  Link::receive( std::move(packet) );
}
void UserNet::feed(void* data, net::BufferStore* bufstore)
{
  // wrap in packet, pass to Link-layer
  driver_hdr& driver = *(driver_hdr*) data;
  auto size = driver.len;

  auto* ptr = (net::Packet*) ((char*) data - sizeof(net::Packet));
  new (ptr) net::Packet(
      sizeof(driver_hdr),
      size,
      sizeof(driver_hdr) + packet_len(),
      bufstore);

  Link::receive(net::Packet_ptr(ptr));
}
void UserNet::write(const void* data, int len)
{
  assert(len >= 0);
  auto buffer = bufstore().get_buffer();
  // wrap in packet, pass to Link-layer
  auto* ptr = (net::Packet*) buffer.addr;
  new (ptr) net::Packet(
      sizeof(driver_hdr),
      len,
      sizeof(driver_hdr) + packet_len(),
      buffer.bufstore);
  // copy data over
  memcpy(ptr->layer_begin(), data, len);
  // send to network stack
  Link::receive(net::Packet_ptr(ptr));
}

// create new packet from nothing
net::Packet_ptr UserNet::create_packet(int link_offset)
{
  auto  buffer = bufstore().get_buffer();
  auto* ptr    = (net::Packet*) buffer.addr;

  new (ptr) net::Packet(
        sizeof(driver_hdr) + link_offset,
        0,
        sizeof(driver_hdr) + packet_len(),
        buffer.bufstore);

  return net::Packet_ptr(ptr);
}
// wrap buffer-length (that should belong to bufferstore) in packet wrapper
