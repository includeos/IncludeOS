//DEBUG: Comment out
#include <iostream>
#include <string>
#include <string.h>

#include "SimplifiedDnsServer.hpp"
#include "SimplifiedDnsClient.hpp"
#include "dnsFormat.hpp"

class SimplifiedDnsClient;

using namespace net;

void SimplifiedDnsServer::setDnsClient(SimplifiedDnsClient* _dnsClient)
{
  dnsClient = _dnsClient;
}

SimplifiedDnsServer::SimplifiedDnsServer()
{
	static const int REPO_SIZE = 10000;
	
  dnsResponsePacket = new DnsResponsePacket();
  dnsResponsePacketChars = reinterpret_cast<uint8_t*> (dnsResponsePacket);
  responsePacketLength = sizeof(DnsResponsePacket);
  // DEBUG:
  std::cout << "Server packet length: " << std::to_string(responsePacketLength) + "\n";
  /*for (uint32_t i = 0; i< responsePacketLength; i++) {
    std::cout << "Byte " + std::to_string(i) + " : " + (char)dnsResponsePacketChars[i] +"\n";
    }*/
  dataStart = sizeof(full_header);
  dnsPacket = new Packet(dnsResponsePacketChars, responsePacketLength, Packet::packet_status::AVAILABLE);
  //packetObjectLength = sizeof(dnsPacket);
  questionStart = dataStart + sizeof(DnsHeaderResponse); // 12
  questionNumberStart = questionStart + 6; // 18
  questionNumberStop = questionNumberStart + 5; //23
  
  // Initializing the dnsRepository
  dnsResponseRepository = new uint8_t[REPO_SIZE * responsePacketLength];
  
  for (uint32_t packetNo = 0; packetNo < REPO_SIZE; packetNo++)
  {
    setServerNumber(packetNo, dnsPacket);
    setArecordIPaddress(packetNo, dnsPacket);
    
    for (uint32_t j = 0; j < responsePacketLength; j++)
    {
      dnsResponseRepository[packetNo * responsePacketLength + j]
		= dnsResponsePacketChars[j];
    }
    //std::strncpy((char*) (dnsResponseRepository+packetNo*responsPacketLength), (char*) dnsResponsePacketChars, responsePacketLength); 
  }
  /*  // DEBUG:
  for (uint32_t i = 0; i< responsePacketLength; i++) {
    std::cout << "Byte " + std::to_string(i) + " : " + std::to_string((char)dnsResponseRepository[0*responsePacketLength+i]) +"\n";
    }
  for (uint32_t i = 0; i< responsePacketLength; i++) {
    std::cout << "Byte " + std::to_string(i) + " : " + std::to_string((char)dnsResponseRepository[22*responsePacketLength+i]) +"\n";
    }
  */
}

SimplifiedDnsServer::~SimplifiedDnsServer()
{
	delete dnsPacket;
	delete dnsResponsePacket;
}

void
SimplifiedDnsServer::setArecordIPaddress(uint32_t serverNo, net::Packet* pckt)
{
  uint8_t bytes [4];
  bytes[0] = (uint8_t) serverNo; // Here the low serverNo goes...
  bytes[1] = (uint8_t) (serverNo >> 8);
  bytes[2] = (uint8_t) (serverNo >> 16);
  bytes[3] = (uint8_t) (serverNo >> 24);
  bytes[3] = 0x0a; // Enforcing 10.0.0.0 prefix
  //bytes[3] = 0x0b; // Debug
  
  for (uint32_t j = 0; j < 4; j++)
  {
    pckt->buffer()[(responsePacketLength)-4 + j] = bytes[j];
  }
}

uint32_t
SimplifiedDnsServer::getArecordIPaddress(Packet * pckt)
{
  uint32_t serverNo = 0;
  uint8_t bytes [4];
  for (uint32_t j=0; j<4 ; j++)
  {
    bytes[j] = pckt->buffer()[(responsePacketLength)-4+j];
  }
  serverNo = serverNo + (bytes[0] <<  0);
  serverNo = serverNo + (bytes[1] <<  8);
  serverNo = serverNo + (bytes[2] << 16);
  serverNo = serverNo + (bytes[3] << 24);
  
  return serverNo;
}

void
SimplifiedDnsServer::setServerNumber(uint32_t serverNo, net::Packet* pckt)
{
  std::string number = std::to_string(serverNo);
  uint32_t len = number.length(); 
  //uint8_t * packetChars = reinterpret_cast<uint8_t*> (dnsPacket->buffer());
  //ASSERT(len <= 5);
  for (uint32_t j=0; j<len ; j++)
  {
    pckt->buffer()[questionNumberStop-len+j] = number[j];
    //packetChars[questionNumberStop-len+j] = number[j];
  }
}

uint32_t
SimplifiedDnsServer::getServerNumber(net::Packet* pckt)
{
  // TODO: use snprintf()
  std::string number = "00000"; // Initial value
  uint32_t len = number.length();
  for (uint32_t j = 0; j < len; j++)
  {
    number[j] = pckt->buffer()[questionNumberStart+j]; 
  }
  number.erase(0,number.find_first_not_of('0') );
  return std::stoi(number);
}
  
//SimplifiedDnsServer::receive(std::shared_ptr<Packet>& pckt) {
uint32_t
SimplifiedDnsServer::receive(net::Packet* pckt)
{
  uint32_t serverNo = getServerNumber(pckt);
  std::cout << "DnsServer::receive: Got " << std::to_string(serverNo) << std::endl;
  
  uint8_t* returnData = dnsResponseRepository+serverNo*responsePacketLength;
  
  responsePacket = new Packet(returnData, responsePacketLength, Packet::packet_status::AVAILABLE);
  
  uint32_t resolvedIP = getArecordIPaddress(responsePacket);

  // DEBUG (output to screen
  std::cout << "DnsServer::receive: Locally stored ip address is " << std::to_string(resolvedIP) << " (";
  // Print out IP
  uint8_t bytes [4];
  bytes[0] = (resolvedIP >>  0) & 0xFF;
  bytes[1] = (resolvedIP >>  8) & 0xFF;
  bytes[2] = (resolvedIP >> 16) & 0xFF;
  bytes[3] = (resolvedIP >> 24) & 0xFF;
  
  for (uint32_t j = 0; j < 4; j++)
  {
    std::cout << std::to_string(bytes[j]) + ".";
  }
  std::cout << ")\n";
  // END (DEBUG)

  dnsClient->receive(responsePacket);
 
  delete responsePacket;
  responsePacket = 0;
}
