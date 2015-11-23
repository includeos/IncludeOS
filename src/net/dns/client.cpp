#include <net/dns/client.hpp>

#include <net/ip4/udp.hpp>
#include <net/dns/dns.hpp>

namespace net
{
  void DNSClient::resolve(IP4::addr dns_server, const std::string& hostname, Stack::resolve_func<IP4> func)
  {
    UDP::port port = 33314; // <-- FIXME: should be automatic port
    auto& sock = stack.udp().bind(port);
    
    // create DNS request
    DNS::Request request;
    char* data = new char[256];
    int   len  = request.create(data, hostname);
    
    // send request to DNS server
    sock.sendto(dns_server, DNS::DNS_SERVICE_PORT, data, len);
    delete[] data;
    
    // wait for response
    // FIXME: WE DO NOT CHECK TRANSACTION IDS HERE (yet), GOD HELP US ALL
    sock.onRead( [this, hostname, request, func]
    (SocketUDP&, IP4::addr, UDP::port, const char* data, int) mutable -> int
    {
      // original request ID = this->id;
      request.parseResponse(data);
      
      // fire onResolve event 
      func(this->stack, hostname, request.getFirstIP4());
      return -1;
    });
  }
}
