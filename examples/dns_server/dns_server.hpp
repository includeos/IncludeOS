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
  static void addMapping(const std::string& key, std::vector<net::IP4::addr> values)
  {
    repository[key] = values;
  }
  
  void start(net::Inet*);
  int listener(std::shared_ptr<net::Packet>&);
  
  /// @brief Populate the registry 
  static void init();  
  
  /// @brief Do a lookup, using a lookup function
  static std::vector<net::IP4::addr>* lookup(const std::string& name);

private:
  net::Inet* network;
  static std::map<std::string, std::vector<net::IP4::addr>> repository;
  
};
  
#endif
