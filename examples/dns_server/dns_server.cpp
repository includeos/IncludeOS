// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015 Oslo and Akershus University College of Applied Sciences
// and Alfred Bratterud
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

#include "dns_server.hpp"

using namespace net;
using namespace std;
eastl::map<eastl::string, eastl::vector<IP4::addr>> DNS_server::repository;

const int NUM=10000;

extern unsigned short ntohs(unsigned short sh);
#define htons ntohs

void DNS_server::init(){
    /// www.google.com ///
  std::vector<IP4::addr> mapping1;
  mapping1.push_back( { 213, 155, 151, 187 } );
  mapping1.push_back( { 213, 155, 151, 185 } );
  mapping1.push_back( { 213, 155, 151, 180 } );
  mapping1.push_back( { 213, 155, 151, 183 } );
  mapping1.push_back( { 213, 155, 151, 186 } );
  mapping1.push_back( { 213, 155, 151, 184 } );
  mapping1.push_back( { 213, 155, 151, 181 } );
  mapping1.push_back( { 213, 155, 151, 182 } );
  addMapping("www.google.com.", mapping1);  
  
  for (int i=0; i<NUM; i++){
    std::vector<IP4::addr> mapping1;
    mapping1.push_back( {10,0,(i >> 8) & 0xFF, i & 0xFF } );
    
    string key = (string("server")+to_string(i))+string(".local.org.");
    //cout << "Adding entry " << key << endl;
    addMapping(key,mapping1);
    
  }
  
}

void DNS_server::start(Inet* net)
{
  cout << "Starting DNS server on port " << DNS::DNS_SERVICE_PORT << ".." << endl;
  this->network = net;
  
  auto del(upstream::from<DNS_server, &DNS_server::listener>(this));
  net->udp_listen(DNS::DNS_SERVICE_PORT, del);
  cout << " ------------- Done. DNS server listening. " << endl;
}

int DNS_server::listener(net::Packet_ptr pckt)
{
  cout << "<DNS SERVER> got packet..." << endl;
  
  UDP::full_header& udp = *(UDP::full_header*) pckt->buffer();
  DNS::header& hdr      = *(DNS::header*) (pckt->buffer() + sizeof(UDP::full_header));
  
  /// create response ///
  
  int packetlen = DNS::createResponse(hdr,
  [this] (const std::string& name) ->
  std::vector<IP4::addr>*
  {
    auto it = repository.find(name);
    if (it == repository.end()) return nullptr;
    return &repository[name];
  });
  
  /// send response back to client ///
  
  // set source & return address
  udp.udp_hdr.dport = udp.udp_hdr.sport;
  udp.udp_hdr.sport = htons(DNS::DNS_SERVICE_PORT);
  udp.udp_hdr.length = htons(sizeof(UDP::udp_header) + packetlen);
  
  // Populate outgoing IP header
  udp.ip_hdr.daddr = udp.ip_hdr.saddr;
  udp.ip_hdr.saddr = network->ip4(ETH0);
  udp.ip_hdr.protocol = IP4::IP4_UDP;
  
  // packet length (??)
  int res = pckt->set_len(sizeof(UDP::full_header) + packetlen); 
  if(!res)
    cout << "<DNS_SERVER> ERROR setting packet length failed" << endl;
  std::cout << "Returning " << packetlen << "b to " << udp.ip_hdr.daddr.str() << std::endl;  
  std::cout << "Full packet size: " << pckt->len() << endl;
  // return packet (as DNS response)
  network->udp_send(pckt);
  
  return 0;
}

std::vector<net::IP4::addr>* 
DNS_server::lookup(const string& name){
  auto it = repository.find(name);
  if (it == repository.end()) return nullptr;
  return &repository[name];
    
}
