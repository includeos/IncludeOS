#ifndef CLASS_IP6_HPP
#define CLASS_IP6_HPP

#include <delegate>
#include <net/class_ethernet.hpp>

namespace net
{
  /** IP6 layer skeleton */
  class IP6
  {
    // Outbound data goes through here
    //Ethernet& _eth;
    
  public:
    /** Handle IPv6 packet. */
    int bottom(std::shared_ptr<Packet>& pckt);
    
    /** Constructor. Requires ethernet to latch on to. */
    //IP6(Ethernet& eth);
    
  };
} // ~net

#endif
