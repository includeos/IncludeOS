
#include <net/dns/query.hpp>

namespace net::dns {

  size_t Query::write(char* buffer) const
  {
    // fill with DNS request data
    Header* hdr = (Header*) buffer;
    hdr->id = htons(this->id);
    hdr->qr = DNS_QR_QUERY;
    hdr->opcode = 0;       // standard query
    hdr->aa = 0;           // not Authoritative
    hdr->tc = DNS_TC_NONE; // not truncated
    hdr->rd = 1; // recursion Desired
    hdr->ra = 0; // recursion not available
    hdr->z  = DNS_Z_RESERVED;
    hdr->ad = 0;
    hdr->cd = 0;
    hdr->rcode = static_cast<uint8_t>(Response_code::NO_ERROR);
    hdr->q_count = htons(1); // only 1 question
    hdr->ans_count  = 0;
    hdr->auth_count = 0;
    hdr->add_count  = 0;

    // point to the query portion
    char* qname = buffer + sizeof(Header);

    // convert host to dns name format
    int namelen = write_formatted_hostname(qname);
    //int namelen = strlen(qname) + 1;

    // set question to Internet A record
    auto* qinfo = (Question*) (qname + namelen);
    qinfo->qtype  = htons(static_cast<uint16_t>(this->rtype));
    qinfo->qclass = htons(static_cast<uint16_t>(Class::INET));

    // return the size of the message to be sent
    return sizeof(Header) + namelen + sizeof(Question);
  }

}
