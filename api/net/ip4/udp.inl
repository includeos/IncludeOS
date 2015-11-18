//-*- C++ -*-
#include <debug>

namespace net
{
  int ignore_udp(Packet_ptr);
  
  inline UDP::UDP(Stack& inet_stack)
      : _network_layer_out(downstream(ignore_udp)),
        stack(inet_stack)
  {
    
  }
  
}
