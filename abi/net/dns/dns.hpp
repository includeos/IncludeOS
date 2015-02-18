#ifndef DNS_CLIENT_HPP
#define DNS_CLIENT_HPP

/**
 * DNS message
 * +---------------------+
 * | Header              |
 * +---------------------+
 * | Question            | the question for the name server
 * +---------------------+
 * | Answer              | RRs answering the question
 * +---------------------+
 * | Authority           | RRs pointing toward an authority
 * +---------------------+
 * | Additional          | RRs holding additional information
 * +---------------------+
 * 
 * DNS header
 * +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 * |                     ID                        |
 * +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 * |QR| Opcode    |AA|TC|RD|RA| Z      |  RCODE    |
 * +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 * |                   QDCOUNT                     |
 * +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 * |                   ANCOUNT                     |
 * +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 * |                   NSCOUNT                     |
 * +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 * |                   ARCOUNT                     |
 * +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 * 
**/

#include <string>
#include <vector>

#include <stdio.h>
#include <string.h>

struct dns_header_t
{
	unsigned short id;       // identification number
	unsigned char rd :1;     // recursion desired
	unsigned char tc :1;     // truncated message
	unsigned char aa :1;     // authoritive answer
	unsigned char opcode :4; // purpose of message
	unsigned char qr :1;     // query/response flag
	unsigned char rcode :4;  // response code
	unsigned char cd :1;     // checking disabled
	unsigned char ad :1;     // authenticated data
	unsigned char z :1;      // reserved, set to 0
	unsigned char ra :1;     // recursion available
	unsigned short q_count;    // number of question entries
	unsigned short ans_count;  // number of answer entries
	unsigned short auth_count; // number of authority entries
	unsigned short add_count;  // number of resource entries
} __attribute__ ((packed));

struct dns_question_t
{
	unsigned short qtype;
	unsigned short qclass;
};

#pragma pack(push, 1)
struct dns_rr_data_t // resource record data
{
	unsigned short type;
	unsigned short _class;
	unsigned int   ttl;
	unsigned short data_len;
};
#pragma pack(pop)

#define DNS_PORT         53

#define DNS_QR_QUERY     0
#define DNS_QR_RESPONSE  1

#define DNS_TC_NONE    0 // no truncation
#define DNS_TC_TRUNC   1 // truncated message

#define DNS_CLASS_INET   1

#define DNS_TYPE_A    1  // A record
#define DNS_TYPE_NS   2  // respect mah authoritah
#define DNS_TYPE_ALIAS 5 // name alias

#define DNS_TYPE_SOA  6  // start of authority zone
#define DNS_TYPE_PTR 12  // domain name pointer
#define DNS_TYPE_MX  15  // mail routing information

#define DNS_Z_RESERVED   0

enum dns_resp_code_t
{
	NO_ERROR     = 0,
	FORMAT_ERROR = 1,
	SERVER_FAIL  = 2,
	NAME_ERROR   = 3,
	NOT_IMPL     = 4, // unimplemented feature
	OP_REFUSED   = 5, // for political reasons
};

struct dns_rr_t // resource record
{
	dns_rr_t(char*& reader, char* buffer);
	
    std::string name;
    std::string rdata;
    dns_rr_data_t resource;
    
    void print();
	
private:
	// read names in 3www6google3com format
	std::string readName(char* reader, char* buffer, int& count);
};

class DnsRequest
{
public:
	int  createRequest(char* buffer, const std::string& hostname);
	bool parseResponse(char* buffer);
	void print(char* buffer);
	
	const std::string& getHostname() const
	{
		return this->hostname;
	}
	
private:
	unsigned short generateID()
	{
		static unsigned short id = 0;
		return ++id;
	}
	void dnsNameFormat(char* dns);
	
	std::string hostname;
	dns_question_t* qinfo;
	
    std::vector<dns_rr_t> answers;
    std::vector<dns_rr_t> auth;
    std::vector<dns_rr_t> addit;
};

#endif
