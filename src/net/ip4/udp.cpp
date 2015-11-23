#define DEBUG
#include <os>
#include <net/ip4/udp.hpp>
#include <net/util.hpp>
#include <memory>

using namespace net;

int UDP::bottom(Packet_ptr pckt)
{
  debug("<UDP handler> Got data");
  std::shared_ptr<PacketUDP> udp = 
      std::static_pointer_cast<PacketUDP> (pckt);
  
  debug("\t Source port: %i, Dest. Port: %i Length: %i\n",
        udp->src_port(), udp->dst_port(), udp->length());
  
  auto it = ports.find(udp->dst_port());
  if (it != ports.end())
  {
    debug("<UDP> Someone's listening to this port. Forwarding...\n");
    return it->second.internal_read(udp);
  }
  
  debug("<UDP> Nobody's listening to this port. Drop!\n");
  return -1;
}

SocketUDP& UDP::bind(port_t port)
{
  debug("<UDP> Listening to port %i\n", port);
  /// ... !!!
  auto it = ports.find(port);
  if (it == ports.end())
  {
    // create new socket
    auto res = ports.emplace(
        std::piecewise_construct,
        std::forward_as_tuple(port),
        std::forward_as_tuple(stack, port));
    it = res.first;
  }
  return it->second;
}
SocketUDP& UDP::bind()
{
  while (true)
  {
    auto it = ports.find(currentPort);
    // if the spot is empty, use it for our socket
    if (it == ports.end())
        return bind(currentPort);
    // otherwise increment the automatic port counter
    currentPort += 1;
    currentPort |= 0x400; // prevent automatic ports under 1024
  }
}

int UDP::transmit(std::shared_ptr<PacketUDP> udp)
{
  debug2("<UDP> Transmitting %i bytes (seg=%i) from %s to %s:%i\n",
        udp->length(), udp->ip4_segment_size(),
        udp->src().str().c_str(),
        udp->dst().str().c_str(), udp->dst_port());
  
  assert(udp->length() >= sizeof(UDP::udp_header));
  assert(udp->protocol() == IP4::IP4_UDP);
  
  Packet_ptr pckt = Packet::packet(udp);
  return _network_layer_out(pckt);
}

namespace net
{
  int ignore_udp(Packet_ptr)
  {
    debug("<UDP->Network> No handler - DROP!\n");
    return 0;
  }
}
