#define DEBUG
#include <os>
#include <net/ip4/udp.hpp>
#include <net/util.hpp>
#include <memory>

using namespace net;

int UDP::bottom(Packet_ptr pckt)
{
  debug("<UDP handler> Got data \n");
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

int UDP::transmit(std::shared_ptr<PacketUDP> udp)
{
  assert(udp->length() >= sizeof(UDP::full_header));
  
  debug("<UDP> Transmitting %i bytes (big-endian 0x%x) to %s:%i \n",
        udp->length(), htons(udp->ip4_header().tot_len),
        udp->dst().str().c_str(), udp->dst_port());
  
  assert(udp->protocol() == IP4::IP4_UDP);
  
  Packet_ptr pckt = udp->packet();
  return _network_layer_out(pckt);
}

namespace net
{
  int ignore_udp(Packet_ptr UNUSED(pckt))
  {
    debug("<UDP->Network> No handler - DROP!\n");
    return 0;
  }
}
