#include <hw/usernet.hpp>
#include <hal/machine.hpp>

constexpr MAC::Addr UserNet::MAC_ADDRESS;

inline static size_t aligned_size_from_mtu(uint16_t mtu) {
  mtu += sizeof(net::Packet);
  if (mtu <= 4096)  return 4096;
  if (mtu <= 8192)  return 8192;
  if (mtu <= 16384) return 16384;
  if (mtu <= 32768) return 32768;
  return 65536;
}

UserNet::UserNet(const uint16_t mtu)
  : Link(Link::Protocol{{this, &UserNet::transmit}, MAC_ADDRESS}),
    mtu_value(mtu), buffer_store(256u, aligned_size_from_mtu(mtu)) {}

UserNet& UserNet::create(const uint16_t mtu)
{
  // the IncludeOS packet communicator
  auto* usernet = new UserNet(mtu);
  // register driver for superstack
  auto driver = std::unique_ptr<hw::Nic> (usernet);
  os::machine().add<hw::Nic> (std::move(driver));
  return *usernet;
}

size_t UserNet::transmit_queue_available()
{
  return buffer_store.available();
}

void UserNet::transmit(net::Packet_ptr packet)
{
  assert(transmit_forward_func);
  transmit_forward_func(std::move(packet));
}
void UserNet::receive(net::Packet_ptr packet)
{
  // wrap in packet, pass to Link-layer
  Link::receive( std::move(packet) );
}
void UserNet::receive(void* data, net::BufferStore* bufstore)
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
void UserNet::receive(const void* data, int len)
{
  assert(len >= 0);
  auto* buffer = new uint8_t[buffer_store.bufsize()];
  // wrap in packet, pass to Link-layer
  auto* ptr = (net::Packet*) buffer;
  new (ptr) net::Packet(
      sizeof(driver_hdr),
      len,
      sizeof(driver_hdr) + packet_len(),
      nullptr);
  // copy data over
  memcpy(ptr->layer_begin(), data, len);
  // send to network stack
  Link::receive(net::Packet_ptr(ptr));
}

// create new packet from nothing
net::Packet_ptr UserNet::create_packet(int link_offset)
{
  auto* buffer = buffer_store.get_buffer();
  auto* ptr    = (net::Packet*) buffer;

  new (ptr) net::Packet(
        sizeof(driver_hdr) + link_offset,
        0,
        sizeof(driver_hdr) + packet_len(),
        &buffer_store);

  return net::Packet_ptr(ptr);
}
