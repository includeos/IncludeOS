// Part of the IncludeOS Unikernel  - www.includeos.org
//
// Copyright 2015 Oslo and Akershus University College of Applied Sciences
// and  Alfred Bratterud. 
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
//     http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.


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
  int listener(net::Packet_ptr);
  
  /// @brief Populate the registry 
  static void init();  
  
  /// @brief Do a lookup, using a lookup function
  static std::vector<net::IP4::addr>* lookup(const std::string& name);

private:
  net::Inet* network;
  static std::map<std::string, std::vector<net::IP4::addr>> repository;
  
};
  
#endif
