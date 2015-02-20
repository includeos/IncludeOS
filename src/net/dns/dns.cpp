#include <net/dns/dns.hpp>

#include <string>

extern unsigned short ntohs(unsigned short sh);
#define htons ntohs
using namespace std;

namespace net
{
  std::string parse_dns_query(unsigned char* c)
  {
    auto tmp = c;
    std::string resp;
    
    while (*(tmp)!=0)
    {
      int len = *tmp++;
      resp.append((char*) tmp, len);
      resp.append(".");
      tmp += len;
    }
    return resp;
  }
  
  int DNS::createResponse(DNS::header& hdr, DNS::lookup_func lookup)
  {
    cout << "Request ID: " << htons(hdr.id);
    cout << "\t Type: " << (hdr.qr ? "RESPONSE" : "QUERY") << endl;  
    
    unsigned short qno = ntohs(hdr.q_count);
    cout << "Questions: " << qno << endl;
    
    char* buffer = (char*) &hdr + sizeof(header);
    
    /// NOTE: ASSUMING 1 QUESTION ///
    char* query = buffer;
    
    std::string parsed_query = parse_dns_query((unsigned char*) query);
    cout << "Question: " << parsed_query << endl;
    
    buffer += parsed_query.size() + 1; // zero-terminated
    
    question& q = *(question*) buffer;
    
    unsigned short qtype  = ntohs(q.qtype);
    unsigned short qclass = ntohs(q.qclass);
    
    cout << "Type: " << DNS::question_string(qtype) << " (" << qtype << ")";
    cout << "\t Class: " << ((qclass == 1) ? "INET" : "Unknown class") << 
        " (" << qclass << ")" << endl;
    
    // go to next question (assuming 1 question!!!!)
    buffer += sizeof(question);
    
    //////////////////////////
    /// RESPONSE PART HERE ///
    //////////////////////////
    
    // initial response size
    unsigned short packetlen = sizeof(header) + 
        sizeof(question) + parsed_query.size() + 1;
    
    // set DNS QR to RESPONSE
    hdr.qr = DNS_QR_RESPONSE;
    hdr.aa = 1; // authoritah
    
    // auth & additional = 0, for now
    hdr.auth_count = 0;
    hdr.add_count  = 0;
    
    std::vector<IP4::addr>* addrs = lookup(parsed_query);
    if (addrs == nullptr)
    {
      // not found
      cout << "*** Could not find: " << parsed_query << endl;
      hdr.ans_count = 0;
      hdr.rcode     = DNS::NO_ERROR;
    }
    else
    {
      cout << "*** Found " << addrs->size() << " results for: " << parsed_query << endl;
      // append answers
      for (auto addr : *addrs)
      {
        cout << "*** Result: " << addr.str() << endl;
        // add query
        int qlen = parsed_query.size() + 1;
        memcpy(buffer, query, qlen);
        buffer += qlen;
        packetlen += qlen; // (!)
        
        // add resource record
        rr_data* data = (rr_data*) buffer;
        
        data->type     = htons(DNS_TYPE_A);
        data->_class   = htons(DNS_CLASS_INET);
        data->ttl      = htons(0x7FFF); // just because
        data->data_len = htons(sizeof(IP4::addr));
        buffer += sizeof(rr_data);
        
        // add resource itself
        *((IP4::addr*) buffer) = addr; // IPv4 address
        buffer += sizeof(IP4::addr);
        
        packetlen += sizeof(rr_data) + sizeof(IP4::addr); // (!)
      } // addr
      
      // set dns header answer count (!)
      hdr.ans_count = htons((addrs->size() & 0xFFFF));
      hdr.rcode     = DNS::NO_ERROR;
    }
    return packetlen;
  }
  
}
