#include <os>
#include <class_dev.hpp>
#include <assert.h>
#include <net/inet>
#include <list>
#include <memory>

// Locals
#include "dns_server.hpp"

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

PacketStore         UDP_store(100,1500);
//SimplifiedDnsServer myDnsServer;
DNS_server myDnsServer;

void Service::start()
{
	std::cout << "*** Service is up - with OS Included! ***" << std::endl;
	std::cout << "Starting DNS prototype\n";
	
	using namespace net;
	
	//auto& mac = Dev::eth(0).mac();
	Inet::ifconfig(
                       net::ETH0,
                       {  10,   0,   0,  2 },  // IP
                       { 255, 255, 240,  0 }); // Netmask
	
        
	
	Inet* inet = Inet::up();
	std::cout << "...Starting UDP server on IP " 
			<< inet->ip4(net::ETH0).str() << std::endl;
	
  /// www.google.com ///
  std::vector<IP4::addr> mapping1;
  mapping1.push_back( {213, 155, 151, 187 } );
  mapping1.push_back( {213, 155, 151, 185 } );
  mapping1.push_back( {213, 155, 151, 180 } );
  mapping1.push_back( {213, 155, 151, 183 } );
  mapping1.push_back( {213, 155, 151, 186 } );
  mapping1.push_back( {213, 155, 151, 184 } );
  mapping1.push_back( {213, 155, 151, 181 } );
  mapping1.push_back( {213, 155, 151, 182 } );
  
  myDnsServer.addMapping("www.google.com.", mapping1);
  ///               ///
  
	myDnsServer.start(inet);
	std::cout << "<DNS SERVER> Listening on UDP port 53" << std::endl;
	
	std::cout << "Service out!" << std::endl;
}
