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
    using Stack = Inet<LINKLAYER, IPV>;
    template <typename IPv>
    using resolve_func = delegate<void(Stack&, const std::string&, typename IPv::addr)>;
    
    virtual const typename IPV::addr& ip_addr() = 0;
    virtual const typename IPV::addr& netmask() = 0;
    virtual const typename IPV::addr& router() = 0;
    virtual const typename LINKLAYER::addr& link_addr() = 0;
    virtual LINKLAYER& link() = 0;
    virtual IPV& ip_obj() = 0;
    virtual TCP& tcp() = 0;
    virtual UDP& udp() = 0;
    
    virtual uint16_t MTU() const = 0;
    
    virtual Packet_ptr createPacket(size_t size) = 0;
    
    virtual void
    resolve(const std::string& hostname,
            resolve_func<IPV>  func) = 0;
    
  private:
    virtual void network_config(
        typename IPV::addr ip, 
        typename IPV::addr nmask, 
        typename IPV::addr router) = 0;
    friend class DHClient;
  };

}

#endif
