#ifndef NET_DNS_CLIENT_HPP
#define NET_DNS_CLIENT_HPP

#include "../inet.hpp"
#include "../ip4.hpp"
#include <map>

namespace net
{
  class DNSClient
  {
  public:
    using Stack = Inet<LinkLayer, IP4>;
    
    DNSClient(Stack& stk)
        : stack(stk)  {}
    
    /**
     * @func  a delegate that provides a hostname and its address, which is 0 if the
     * name @hostname was not found. Note: Test with INADDR_ANY for a 0-address.
    **/
    void resolve(IP4::addr dns_server,
                 const std::string& hostname, 
                 Stack::resolve_func<IP4> func);
    
  private:
    Stack& stack;
    std::map<std::string, IP4::addr> cache;
    std::map<IP4::addr, std::string> rev_cache;
  };
}

#endif
