#include <os>
#include <class_dev.hpp>
#include <assert.h>
#include <net/inet>
#include <list>
#include <memory>

#include "SimplifiedDnsServer.hpp"
#include "SimplifiedDnsClient.hpp"

class PacketStore
{
public:
  std::shared_ptr<net::Packet> getPacket()
  {
    if (_queue.empty())
       panic("Packet store: out of packets");
    auto elt = *_queue.begin();
    _queue.pop_front();
    _queue.push_back(elt);
    return elt;
  }
  
  PacketStore(uint16_t n, uint32_t size):
    _n(n), _bufsize(size), _pool(nullptr)
  {       
    this->_pool = new uint8_t[n * size];
    
    for(int i = 0; i < _n; i++)
    {
      _queue.push_back
        (std::make_shared<net::Packet>(&_pool[i * size], size, net::Packet::AVAILABLE));
    }
        
    printf("<PacketStore> Allocated %li byte buffer pool for packets \n",n*size);
  }

  //PacketStore(int n, int size, caddr_t pool);
  
private:
	uint16_t _n;
	uint32_t _bufsize;
	uint8_t* _pool;
	std::list<std::shared_ptr<net::Packet> > _queue;
};

PacketStore UDP_store(100,1500);

void Service::start()
{
	std::cout << "*** Service is up - with OS Included! ***" << std::endl;
	std::cout << "Starting DNS prototype\n";
	
	using namespace net;
	
	auto& mac = Dev::eth(0).mac();
	Inet::ifconfig(
		net::ETH0,
		{ mac.part[2], mac.part[3], mac.part[4], mac.part[5] },
		{255, 255, 0, 0});
	
	//Inet* net 
	std::shared_ptr<Inet> net(Inet::up());
	
	std::cout << "...Starting UDP server on IP " 
			<< net->ip4(net::ETH0).str() << std::endl;
	
	SimplifiedDnsServer myDnsServer;
	SimplifiedDnsClient myDnsClient;
	
	myDnsServer.setDnsClient(&myDnsClient);
	myDnsClient.setDnsServer(&myDnsServer);
	
	myDnsClient.requestNames((uint32_t) 3);
	
	
	//A one-way UDP server (a primitive test)
	net->udp_listen(8080,
	[net] (std::shared_ptr<net::Packet> pckt)
	{
		UDP::full_header* full_hdr = (UDP::full_header*) pckt->buffer();
		UDP::udp_header*  hdr = &full_hdr->udp_hdr;
		
		int data_len = __builtin_bswap16(hdr->length) - sizeof(UDP::udp_header);
		auto data_loc = pckt->buffer() + sizeof(UDP::full_header);
		
		// Netcat doesn't necessariliy zero-pad the string in UDP
		// ... But this buffer is const
		// auto data_end = data + hdr->length - sizeof(UDP::udp_header);
		// *data_end = 0; 
		
		debug("<APP SERVER> Got %i b of data (%i b frame) from %s:%i -> %s:%i\n",
            data_len, len, full_hdr->ip_hdr.saddr.str().c_str(), 
            __builtin_bswap16(hdr->sport),
            full_hdr->ip_hdr.daddr.str().c_str(), 
            __builtin_bswap16(hdr->dport));
      
		//printf("buf@0x%lx ",   (uint32_t) buf);
		printf("UDP from %s ", full_hdr->ip_hdr.saddr.str().c_str());
		
		/*
		printf("Got '");
		// Print the input
		for (int i = 0; i < data_len; i++)
			printf("%c", data_loc[i]);
		
		printf("' UDP data  from %s. (str: %s) \n", 
			full_hdr->ip_hdr.saddr.str().c_str(),data_loc);
		*/
		
		// Craft response
		std::string response((const char*) data_loc, data_len);
       
		/*
		bufsize = response.size() + sizeof(UDP::full_header);
		
		// Ethernet padding if necessary
		if (bufsize < Ethernet::minimum_payload)
			bufsize = Ethernet::minimum_payload;
		
		delete[] buf;
		buf = new uint8_t[bufsize]; 
		*/
		
		auto pckt_out = UDP_store.getPacket();
		auto buf = pckt_out->buffer();
		
		strncpy((char*) buf + sizeof(UDP::full_header), response.c_str(), data_len);
		buf[data_len-1] = 0;
		
		debug("Reply: '%s'\n", buf + sizeof(UDP::full_header));
		
		// Respond
		debug("<APP SERVER> Sending %li b wrapped in %i b buffer \n",
             response.size(), bufsize);
		
		/** Populate outgoing UDP header */
		UDP::full_header* full_hdr_out = (UDP::full_header*) buf;
		full_hdr_out->udp_hdr.dport = hdr->sport;
		full_hdr_out->udp_hdr.sport = hdr->dport;
		full_hdr_out->udp_hdr.length = __builtin_bswap16(data_len);
		
		/** Populate outgoing IP header */
		full_hdr_out->ip_hdr.saddr = full_hdr->ip_hdr.daddr;
		full_hdr_out->ip_hdr.daddr = full_hdr->ip_hdr.saddr;
		full_hdr_out->ip_hdr.protocol = IP4::IP4_UDP;
		
		/*auto pckt_out = std::make_shared<Packet>
			(Packet(buf, bufsize, Packet::DOWNSTREAM));*/
		net->udp_send(pckt_out);
		
		return 0;
	});
	
	std::cout << "<APP SERVER> Listening to UDP port 8080 " << std::endl;
	
	std::cout << "Service out!" << std::endl;
}
