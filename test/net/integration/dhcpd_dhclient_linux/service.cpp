
#include <service>
#include <net/interfaces>
#include <net/dhcp/dhcpd.hpp>
#include <list>

std::unique_ptr<net::dhcp::DHCPD> server;

void Service::start(const std::string&)
{
  using namespace net;
  using namespace dhcp;

  // Server

  auto& inet = Interfaces::get(0);
  inet.network_config(
    { 10,200,0,1 },     // IP
    { 255,255,0,0 },    // Netmask
    { 10,0,0,1 },       // Gateway
    { 8,8,8,8 });       // DNS

  IP4::addr pool_start{10,200,100,20};
  IP4::addr pool_end{10,200,100,30};
  server = std::make_unique<DHCPD>(inet.udp(), pool_start, pool_end);
  server->listen();

  INFO("<Service>", "Service started");
}
