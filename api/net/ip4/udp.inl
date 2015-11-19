#include <debug>

namespace net
{
  int ignore_udp(Packet_ptr);
  
  inline UDP::UDP(Inet<LinkLayer,IP4>& inet)
      : _network_layer_out(downstream(ignore_udp))
  {
    this->_local_ip = inet.ip_addr();
  }
  
}
