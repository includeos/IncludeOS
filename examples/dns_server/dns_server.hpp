#include <net/inet>
#include <net/dns/dns.hpp>


class DNS_server{
  
public:
  void start(net::Inet*);
  int listener(std::shared_ptr<net::Packet>&);
  
};
  
