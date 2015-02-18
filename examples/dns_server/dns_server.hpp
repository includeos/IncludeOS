#ifndef DNS_SERVER_HPP
#define DNS_SERVER_HPP

#include <net/dns/dns.hpp>
#include <net/inet>

#include <string>
#include <vector>
#include <map>

class DNS_server
{
public:
  void addMapping(const std::string& key, std::vector<net::IP4::addr> values)
  {
    lookup[key] = values;
  }
  
  void start(net::Inet*);
  int listener(std::shared_ptr<net::Packet>&);
  
private:
  net::Inet* network;
  std::map<std::string, std::vector<net::IP4::addr>> lookup;
  
};
  
#endif
