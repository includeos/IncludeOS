#include "dns.hpp"

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

dns_rr_t::dns_rr_t(char*& reader, char* buffer)
{
	int stop;
	
	this->name = readName(reader, buffer, stop);
	reader += stop;
	
	this->resource = *(dns_rr_data_t*) reader;
	reader += sizeof(dns_rr_data_t);
	
	// if its an ipv4 address
	if (ntohs(resource.type) == DNS_TYPE_A)
	{
		int len = ntohs(resource.data_len);
		
		this->rdata = std::string(reader, len);
		reader += len;
	}
	else
	{
		this->rdata = readName(reader, buffer, stop);
		reader += stop;
	}
}

void dns_rr_t::print()
{
	printf("Name: %s ", name.c_str());
	switch (ntohs(resource.type))
	{
	case DNS_TYPE_A:
		{
			long* p    = (long*) rdata.c_str();
			char* addr = inet_ntoa_simple(*p);
			printf("has IPv4 address: %s", addr);
			free(addr);
		}
		break;
	case DNS_TYPE_ALIAS:
		printf("has alias: %s", rdata.c_str());
		break;
	case DNS_TYPE_NS:
		printf("has authoritative nameserver : %s", rdata.c_str());
		break;
	default:
		printf("has unknown resource type: %d", ntohs(resource.type));
	}
	printf("\n");
}

std::string dns_rr_t::readName(char* reader, char* buffer, int& count)
{
	std::string name(256, '\0');
	unsigned p = 0;
	unsigned offset = 0;
	bool jumped = false;
	
	count = 1;
	unsigned char* ureader = (unsigned char*) reader;
	
	while (*ureader)
	{
		if (*ureader >= 192)
		{
			offset = (*ureader) * 256 + *(ureader+1) - 49152; // = 11000000 00000000
			ureader = (unsigned char*) buffer + offset - 1;
			jumped = true; // we have jumped to another location so counting wont go up!
		}
		else
		{
			name[p++] = *ureader;
		}
		ureader++;
		
		// if we havent jumped to another location then we can count up
		if (jumped == false) count++;
	}
	name.resize(p);
	
	// number of steps we actually moved forward in the packet
	if (jumped)
		count++;
	
	// now convert 3www6google3com0 to www.google.com
	int len = p; // same as name.size()
	int i;
	for(i = 0; i < len; i++)
	{
		p = name[i];
		
		for(unsigned j = 0; j < p; j++)
		{
			name[i] = name[i+1];
			i++;
		}
		name[i] = '.';
	}
	name[i - 1] = '\0'; // remove the last dot
	return name;
	
} // readName()

int DnsRequest::createRequest(char* buffer, const std::string& hostname)
{
	this->hostname = hostname;
	this->answers.clear();
	this->auth.clear();
	this->addit.clear();
	
	// fill with DNS request data
	dns_header_t* dns = (dns_header_t*) buffer;
	dns->id = htons(generateID());
	dns->qr = DNS_QR_QUERY;
	dns->opcode = 0;       // standard query
	dns->aa = 0;           // not Authoritative
	dns->tc = DNS_TC_NONE; // not truncated
	dns->rd = 1; // recursion Desired
	dns->ra = 0; // recursion not available
	dns->z  = DNS_Z_RESERVED;
	dns->ad = 0;
	dns->cd = 0;
	dns->rcode = dns_resp_code_t::NO_ERROR;
	dns->q_count = htons(1); // only 1 question
	dns->ans_count  = 0;
	dns->auth_count = 0;
	dns->add_count  = 0;
	
    // point to the query portion
	char* qname = buffer + sizeof(dns_header_t);
	
	// convert host to dns name format
	dnsNameFormat(qname);
	// length of dns name
	int namelen = strlen(qname) + 1;
	
	// set question to Internet A record
	this->qinfo   = (dns_question_t*) (qname + namelen);
	qinfo->qtype  = htons(DNS_TYPE_A); // ipv4 address
	qinfo->qclass = htons(DNS_CLASS_INET);
	
	// return the size of the message to be sent
	return sizeof(dns_header_t) + namelen + sizeof(dns_question_t);
}

// parse received message (as put into buffer)
bool DnsRequest::parseResponse(char* buffer)
{
	dns_header_t* dns = (dns_header_t*) buffer;
	
	// move ahead of the dns header and the query field
	char* reader = ((char*) this->qinfo) + sizeof(dns_question_t);
	
	// parse answers
    for(int i = 0; i < ntohs(dns->ans_count); i++)
		answers.emplace_back(reader, buffer);
	
    // parse authorities
    for (int i = 0; i < ntohs(dns->auth_count); i++)
        auth.emplace_back(reader, buffer);
	
    // parse additional
    for (int i = 0; i < ntohs(dns->add_count); i++)
		addit.emplace_back(reader, buffer);
	
	return true;
}

void DnsRequest::print(char* buffer)
{
	dns_header_t* dns = (dns_header_t*) buffer;
	
	printf(" %d questions\n", ntohs(dns->q_count));
	printf(" %d answers\n",   ntohs(dns->ans_count));
	printf(" %d authoritative servers\n", ntohs(dns->auth_count));
	printf(" %d additional records\n\n",  ntohs(dns->add_count));
	
	// print answers
	for (auto& answer : answers)
		answer.print();
	
	// print authorities
	for (auto& a : auth)
		a.print();
	
	// print additional resource records
	for (auto& a : addit)
		a.print();
	
	printf("\n");
}

// convert www.google.com to 3www6google3com
void DnsRequest::dnsNameFormat(char* dns)
{
    int lock = 0;
	
    std::string copy = this->hostname + ".";
    int len = copy.size();
    
    for(int i = 0; i < len; i++)
    {
        if (copy[i] == '.')
        {
            *dns++ = i - lock;
            for(; lock < i; lock++)
            {
                *dns++ = copy[lock];
            }
            lock++;
        }
    }
    *dns++ = '\0';
}
