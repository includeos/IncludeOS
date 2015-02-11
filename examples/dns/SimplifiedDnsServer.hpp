#ifndef SIMPLIFIEDDNSSERVER_HPP
#define SIMPLIFIEDDNSSERVER_HPP

// g++ compilation problem with shared_ptr?
#include <memory> // for shared_ptr
#include <net/inet>

#include "dnsFormat.hpp"
#include <stdint.h>

//DEBUG: Comment out
#include "dnsFormat.hpp"

//struct DnsQuery;
struct DnsResponse;
class SimplifiedDnsClient;

class SimplifiedDnsServer
{
public:
	SimplifiedDnsServer();
	~SimplifiedDnsServer();
	
	//uint32_t receive(std::shared_ptr<Packet>& pckt);
	uint32_t receive(net::Packet* pckt);
	void setDnsClient(SimplifiedDnsClient * _dnsClient);
	
private:
	void setServerNumber(uint32_t serverNo, net::Packet* pckt);
	uint32_t getServerNumber(net::Packet* pckt);
	
	void setArecordIPaddress(uint32_t serverNo, net::Packet* pckt);
	uint32_t getArecordIPaddress(net::Packet* pckt);
	
	//Response packet
	net::Packet* dnsPacket;
	net::Packet* responsePacket;
	net::Packet* requestPacket;
	DnsResponsePacket* dnsResponsePacket;
	
	uint8_t* dnsResponsePacketChars; // = reinterpret_cast<uint8_t*> (dnsQuery);
	uint32_t responsePacketLength; // = sizeof(DnsResponsePacket);
	uint32_t dataStart; // = sizeof(full_header);
	uint32_t questionStart;
	uint32_t questionNumberStart;
	uint32_t questionNumberStop;
	
	// uint32_t packetObjectLength;
	SimplifiedDnsClient* dnsClient;
	uint8_t* dnsResponseRepository;
};

#endif
