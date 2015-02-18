#include "dns_server.hpp"

using namespace net;
using namespace std;

void DNS_server::start(Inet* net){
  cout << "DNS Starting .. " << endl;
  
  
  auto del(upstream::from<DNS_server,&DNS_server::listener>(this));
  
  net->udp_listen(DNS_PORT,del);
  
  
}

string parse_dns_query(unsigned char* c){
  auto tmp = c;
  string resp;
  
  while(*(tmp)!=0){
    int len = *tmp++;
    resp.append((char*)tmp,len);
    resp.append(".");
    tmp+=len;
  }
  
  return resp;
  
}


int DNS_server::listener(std::shared_ptr<net::Packet>& pckt){
  cout << "<DNS SERVER> got packet..." << endl;
  
  DNS::full_header* full_hdr = (DNS::full_header*)pckt->buffer();
  DNS::header& hdr = full_hdr->dns_header;
  
  cout << "Request ID: " << hdr.id << endl;
  cout << "Type: " << (hdr.qr ? "RESPONSE" : "QUERY") << endl;  
  cout << "q_count: " << __builtin_bswap16(hdr.q_count) << endl;
  
  char* query = (char*)full_hdr + sizeof(DNS::full_header);
  
  cout << "Query: " << (const char*)query << endl;
  string parsed_query {parse_dns_query((unsigned char*)query)};
  cout << "Parsed: " << parsed_query << endl;
  
  cout << "Query length: " << strlen(query) 
       << " string: " << parsed_query.size()  << endl;
  cout << "Packet size: " << pckt->len() << endl;
  
  
  
  return 0;
}
