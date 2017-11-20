#include <service>
#include "async_device.hpp"
static std::unique_ptr<Async_device> dev2;

void Service::start()
{
  extern void create_network_device(int N, const char* route, const char* ip);
  create_network_device(0, "10.0.0.0/24", "10.0.0.1");

  // Get the first IP stack configured from config.json
  auto& inet = net::Super_stack::get<net::IP4>(0);
  inet.network_config({10,0,0,42}, {255,255,255,0}, {10,0,0,1});

  dev2 = std::make_unique<Async_device> (1500);
  dev2->set_transmit(
    [] (net::Packet_ptr) {
      printf("Inside network dropping packet\n");
    });

  extern void register_plugin_nacl();
  register_plugin_nacl();
}
