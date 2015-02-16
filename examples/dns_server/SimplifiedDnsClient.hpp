#ifndef SIMPLFIEDDNSCLIENT_HPP
#define SIMPLFIEDDNSCLIENT_HPP

#include <net/inet>
#include "dnsFormat.hpp"
//#include "packet.hpp"
#include <stdint.h>

struct DnsQuery;
class SimplifiedDnsServer;

class SimplifiedDnsClient
{
public:
	SimplifiedDnsClient();
	~SimplifiedDnsClient();
	
	uint32_t requestNames(uint32_t numberOfRequest);
	void setDnsServer(SimplifiedDnsServer * _dnsServer);
	uint32_t receive(net::Packet* pckt);
	
private:
	
	void setServerNumber(uint32_t serverNo, net::Packet* pckt);
	uint32_t getServerNumber(net::Packet* pckt);
	void setArecordIPaddress(uint32_t serverNo, net::Packet* pckt);
	uint32_t getArecordIPaddress(net::Packet* pckt);
	
	//DnsQuery dnsQuery1;
	net::Packet* dnsPacket;
	DnsQueryPacket* dnsQueryPacket;
	uint8_t* dnsQueryPacketChars; // = reinterpret_cast<uint8_t*> (dnsQuery);
	uint32_t responsePacketLength; // = sizeof(DnsQueryPacket);
	uint32_t queryPacketLength; // = sizeof(DnsQueryPacket);
	uint32_t dataStart; // = sizeof(full_header);
	uint32_t questionStart;
	uint32_t questionNumberStart;
	uint32_t questionNumberStop;
	
	SimplifiedDnsServer* dnsServer;
};

#endif
