#include <service>
#include "async_device.hpp"
#include <hw/devices.hpp>
static std::unique_ptr<Async_device> dev2;

void Service::start()
{
  // linux tap device
  extern void create_network_device(int N, const char* route, const char* ip);
  create_network_device(0, "10.0.0.0/24", "10.0.0.1");
  // in-memory network device
  dev2 = std::make_unique<Async_device> (1500);
  dev2->set_transmit(
    [] (net::Packet_ptr) {
      printf("Inside network dropping packet\n");
    });
  hw::Devices::register_device<hw::Nic> (std::unique_ptr<hw::Nic> (dev2->get_driver()));

  // Get the first IP stack configured from config.json
  auto& inet = net::Interfaces::get(0);
  inet.network_config({10,0,0,42}, {255,255,255,0}, {10,0,0,1});

  extern void register_plugin_nacl();
  register_plugin_nacl();
}
