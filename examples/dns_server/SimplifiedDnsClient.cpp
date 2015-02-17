//DEBUG: Comment out
#include <iostream>
#include <stdint.h>
#include <string>
#include <net/inet>

#include "SimplifiedDnsClient.hpp"
#include "SimplifiedDnsServer.hpp"

using namespace net;

void SimplifiedDnsClient::setDnsServer(SimplifiedDnsServer* server)
{
	this->dnsServer = server;
}

SimplifiedDnsClient::SimplifiedDnsClient()
{
  dnsQueryPacket = new DnsQueryPacket();
  dnsQueryPacketChars = reinterpret_cast<uint8_t*> (dnsQueryPacket);
  queryPacketLength = sizeof(DnsQueryPacket);
  responsePacketLength = sizeof(DnsResponsePacket);
  std::cout << "Client packet length: " << std::to_string(queryPacketLength) + "\n";
  dataStart = sizeof(full_header);
  dnsPacket = new Packet(dnsQueryPacketChars, queryPacketLength, Packet::packet_status::AVAILABLE);
  questionStart = dataStart + sizeof(DnsHeaderQuery); // 12
  questionNumberStart = questionStart + 6; // 18
  questionNumberStop = questionNumberStart + 5; //23
}

SimplifiedDnsClient::~SimplifiedDnsClient()
{
  delete dnsPacket;
  delete dnsQueryPacket;
}

void
SimplifiedDnsClient::setServerNumber(uint32_t serverNo, Packet* pckt)
{
  std::string number = std::to_string(serverNo);
  uint32_t len = number.length(); 
  //uint8_t * packetChars = reinterpret_cast<uint8_t*> (dnsPacket->buffer());
  //ASSERT(len <= 5);
  for (uint32_t j = 0; j < len; j++)
  {
    pckt->buffer()[questionNumberStop-len+j] = number[j];
    //packetChars[questionNumberStop-len+j] = number[j];
  }
}

uint32_t
SimplifiedDnsClient::getServerNumber(Packet* pckt)
{
  // TODO: use snprintf()
  std::string number = "00000"; // Initial value
  uint32_t len = number.length();
  
  for (uint32_t j=0; j < len ; j++)
  {
    number[j] = pckt->buffer()[questionNumberStart+j]; 
  }
  number.erase(0,number.find_first_not_of('0') );
  return std::stoi(number);
}


void
SimplifiedDnsClient::setArecordIPaddress(uint32_t serverNo, Packet* pckt)
{
	for (uint32_t j = 0; j < 4; j++)
	{
		pckt->buffer()[pckt->len()-4 + j] = 
			(serverNo >> (8 * j)) & 0xFF;
	}
	// Enforcing 10.0.0.0 prefix
	pckt->buffer()[pckt->len()-1] = 0x0a;
}

uint32_t
SimplifiedDnsClient::getArecordIPaddress(Packet* pckt)
{
	uint32_t serverNo = 0;
	
	for (size_t j = 0; j < 4; j++)
	{
		uint8_t B = pckt->buffer()[pckt->len()-4 + j];
		serverNo += B << (8 * j);
	}
	return serverNo;
}

uint32_t
SimplifiedDnsClient::requestNames(uint32_t numberOfRequests)
{
  uint32_t responsesReceived = 0;
  std::cout << "SimplifiedDnsClient::requestNames: Started...\n";
  for (uint32_t i=0; i<numberOfRequests; i++)
  {
    std::cout << "------------------------------------------------\n";
    setServerNumber(i + 1, dnsPacket);

    std::cout << "SimplifiedDnsClient::requestNames: Sending request..\n";

    // SEND PACKET!
    dnsServer->receive(dnsPacket);
    // int UDP::transmit(std::shared_ptr<Packet>& pckt){...
	std::shared_ptr<Packet> pckt(dnsPacket);
	net->udp_send(pckt);
	
    responsesReceived++;
  }
  std::cout << "SimplifiedDnsClient::requestNames: Finished...\n";
  return responsesReceived;
}

uint32_t
SimplifiedDnsClient::receive(Packet* pckt)
{
  std::cout << "SimplifiedDnsClient::receive: entered...";
  uint32_t serverNo = getServerNumber(pckt);
  std::cout << "SimplifiedDnsClient::receive: Got server number " + std::to_string(serverNo) + "\n";
  
  uint32_t resolvedIP = getArecordIPaddress(pckt);
  
  // DEBUG (output to screen
  std::cout << "SimplifiedDnsClient::receive: Resolved servername by DNS, and got IP address: " << std::to_string(resolvedIP) << " (";
  
  // Print out IP
  std::cout << IP4::addr { whole: resolvedIP }.str() << ")" << std::endl;
}
