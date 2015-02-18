#include "dns_server.hpp"

using namespace net;
using namespace std;

void DNS_server::start(Inet* net)
{
  cout << "Starting DNS server on port " << DNS::DNS_SERVICE_PORT << ".." << endl;
  this->network = net;
  
  auto del(upstream::from<DNS_server, &DNS_server::listener>(this));
  net->udp_listen(DNS::DNS_SERVICE_PORT, del);
}

// cheap implementation of ntohs/htons
unsigned short ntohs(unsigned short sh)
{
	unsigned char* B = (unsigned char*) &sh;
	
	return  ((0xff & B[0]) << 8) |
			((0xff & B[1]));
}
#define htons ntohs

// cheap implementation of inet_ntoa
char* inet_ntoa_simple(unsigned address)
{
	char* buffer = new char[16];
	
	unsigned char* B = (unsigned char*) &address;
	sprintf(buffer, "%d.%d.%d.%d", B[0], B[1], B[2], B[3]);
	
	return buffer;
}

string parse_dns_query(unsigned char* c)
{
  auto tmp = c;
  string resp;
  
  while (*(tmp)!=0)
  {
    int len = *tmp++;
    resp.append((char*)tmp,len);
    resp.append(".");
    tmp+=len;
  }
  return resp;
}

int DNS_server::listener(std::shared_ptr<net::Packet>& pckt)
{
  cout << "<DNS SERVER> got packet..." << endl;
  
  DNS::full_header* full_hdr = (DNS::full_header*)pckt->buffer();
  DNS::header& hdr = full_hdr->dns_header;
  
  cout << "Request ID: " << hdr.id;
  cout << "\t Type: " << (hdr.qr ? "RESPONSE" : "QUERY") << endl;  
  
  unsigned short qno = ntohs(hdr.q_count);
  cout << "Questions: " << qno << endl;
  
  char* buffer = (char*) full_hdr + sizeof(DNS::full_header);
  
  /// NOTE: ASSUMING 1 QUESTION ///
  char* query = buffer;
  
  string parsed_query = parse_dns_query((unsigned char*) query);
  cout << "Question: " << parsed_query << endl;
  
  buffer += parsed_query.size() + 1; // zero-terminated
  
  DNS::question& q = *(DNS::question*) buffer;
  
  unsigned short qtype  = ntohs(q.qtype);
  unsigned short qclass = ntohs(q.qclass);
  
  cout << "Type: " << DNS::question_string(qtype) << " (" << qtype << ")";
  cout << "\t Class: " << ((qclass == 1) ? "INET" : "Unknown class") << 
      " (" << qclass << ")" << endl;
  
  // go to next question (assuming 1 question!!!!)
  buffer += sizeof(DNS::question);
  
  //////////////////////////
  /// RESPONSE PART HERE ///
  //////////////////////////
  
  // initial response size
  unsigned short packetlen = sizeof(DNS::full_header) + 
      sizeof(DNS::question) + parsed_query.size() + 1;
  
  // set DNS QR to RESPONSE
  hdr.qr = DNS_QR_RESPONSE;
  
  // auth & additional = 0, for now
  hdr.auth_count = 0;
  hdr.add_count  = 0;
  
  auto it = lookup.find(parsed_query);
  if (it == lookup.end())
  {
    // not found
    cout << "*** Could not find: " << parsed_query << endl;
    hdr.ans_count = 0;
  }
  else
  {
    cout << "*** Found mapping for: " << parsed_query << endl;
    // append answers
    std::vector<IP4::addr>& addrs = lookup[parsed_query];
    for (auto addr : addrs)
    {
      cout << "*** Result: " << addr.str() << endl;
      // add query
      int qlen = parsed_query.size() + 1;
      memcpy(buffer, query, qlen);
      buffer += qlen;
      packetlen += qlen; // (!)
      
      // add resource record
      DNS::rr_data* data = (DNS::rr_data*) buffer;
      
      data->type     = htons(DNS_TYPE_A);
      data->_class   = htons(DNS_CLASS_INET);
      data->ttl      = 0xFFFF; // just because
      data->data_len = sizeof(IP4::addr);
      buffer += sizeof(DNS::rr_data);
      
      // add resource itself
      *((IP4::addr*) buffer) = addr; // IPv4 address
      buffer += sizeof(IP4::addr);
      
      packetlen += sizeof(DNS::rr_data) + sizeof(IP4::addr); // (!)
    } // addr
    
    // set dns header answer count (!)
    hdr.ans_count = addrs.size();
  }
  
  // send response back to client
  UDP::full_header& udp = full_hdr->full_udp_header;
  
  // set source & return address
  udp.udp_hdr.dport = htons(DNS::DNS_SERVICE_PORT);
  udp.udp_hdr.sport = htons(DNS::DNS_SERVICE_PORT);
  udp.udp_hdr.length = htons(packetlen);
  
  // Populate outgoing IP header
  udp.ip_hdr.daddr = udp.ip_hdr.saddr;
  udp.ip_hdr.saddr = network->ip4(ETH0);
  udp.ip_hdr.protocol = IP4::IP4_UDP;
  
  // packet length (??)
  int res = pckt->set_len(packetlen + sizeof(IP4::full_header)); 
  if(!res)
    cout << "<DNS_SERVER> ERROR setting packet length failed" << endl;
  std::cout << "Returning " << packetlen << "b to " << udp.ip_hdr.daddr.str() << std::endl;  
  std::cout << "Full packet size: " << pckt->len() << endl;
  // return packet (as DNS response)
  network->udp_send(pckt);
  
  return 0;
}
