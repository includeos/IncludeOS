#ifndef NET_INET_HPP
#define NET_INET_HPP

#include <net/inet_common.hpp>

namespace net {
  
  class TCP;
  class UDP;

  /** An abstract IP-stack interface  */  
  template <typename LINKLAYER, typename IPV >
  class Inet {
  public:
    
    virtual const typename IPV::addr& ip_addr() = 0;
    virtual const typename IPV::addr& netmask() = 0;
    virtual const typename LINKLAYER::addr& link_addr() = 0;
    virtual LINKLAYER& link() = 0;
    virtual IPV& ip_obj() = 0;
    virtual TCP& tcp() = 0;
    virtual UDP& udp() = 0;
    
    virtual Packet_ptr createPacket(size_t size) = 0;
    
  };

}

#endif
